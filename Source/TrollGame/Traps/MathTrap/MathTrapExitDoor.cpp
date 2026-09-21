// Copyright Epic Games, Inc. All Rights Reserved.

#include "MathTrapExitDoor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "TimerManager.h"

AMathTrapExitDoor::AMathTrapExitDoor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	DoorFrameMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorFrameMesh"));
	DoorFrameMesh->SetupAttachment(RootComponent);
	DoorFrameMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(DoorFrameMesh);
	DoorMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	StatusText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StatusText"));
	StatusText->SetupAttachment(RootComponent);
	StatusText->SetRelativeLocation(FVector(0.0f, 0.0f, 220.0f));
	StatusText->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	StatusText->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	StatusText->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	StatusText->SetWorldSize(36.0f);
	StatusText->SetTextRenderColor(FColor(255, 40, 40, 255)); // Red
	StatusText->SetText(FText::FromString(TEXT("LOCKED")));

	OpenYawAngle = -90.0f;
	OpenSpeed = 4.0f;
	bIsOpen = false;
	bIsOpening = false;
}

void AMathTrapExitDoor::BeginPlay()
{
	Super::BeginPlay();

	// 1. Auto-discover Left and Right door components by name/label if not manually assigned
	if (!LeftDoorComponent || !RightDoorComponent)
	{
		TArray<USceneComponent*> Comps;
		GetComponents<USceneComponent>(Comps);
		for (USceneComponent* Comp : Comps)
		{
			if (!Comp || Comp == SceneRoot || Comp == DoorFrameMesh || Comp == StatusText) continue;
			FString CompName = Comp->GetName();

			if (!LeftDoorComponent && (CompName.Contains(TEXT("left"), ESearchCase::IgnoreCase) || CompName.Contains(TEXT("Door_1_left"), ESearchCase::IgnoreCase) || CompName.Contains(TEXT("Door_left"), ESearchCase::IgnoreCase)))
			{
				LeftDoorComponent = Comp;
			}
			else if (!RightDoorComponent && (CompName.Contains(TEXT("right"), ESearchCase::IgnoreCase) || CompName.Contains(TEXT("Door_1_right"), ESearchCase::IgnoreCase) || CompName.Contains(TEXT("Door_right"), ESearchCase::IgnoreCase)))
			{
				RightDoorComponent = Comp;
			}
		}
	}

	// 2. Check attached child actors if components were not found directly
	if (!LeftDoorComponent || !RightDoorComponent)
	{
		TArray<AActor*> AttachedActors;
		GetAttachedActors(AttachedActors);
		for (AActor* Child : AttachedActors)
		{
			if (!Child) continue;
			FString ChildName = Child->GetActorLabel();
			if (ChildName.IsEmpty()) ChildName = Child->GetName();

			if (!LeftDoorComponent && ChildName.Contains(TEXT("left"), ESearchCase::IgnoreCase))
			{
				LeftDoorComponent = Child->GetRootComponent();
			}
			else if (!RightDoorComponent && ChildName.Contains(TEXT("right"), ESearchCase::IgnoreCase))
			{
				RightDoorComponent = Child->GetRootComponent();
			}
		}
	}

	// 3. Fallback: if 2 static mesh components exist, pick smaller Y/X as left and larger Y/X as right
	if (!LeftDoorComponent || !RightDoorComponent)
	{
		TArray<UStaticMeshComponent*> Meshes;
		GetComponents<UStaticMeshComponent>(Meshes);
		TArray<UStaticMeshComponent*> PanelMeshes;
		for (UStaticMeshComponent* SM : Meshes)
		{
			if (SM && SM != DoorFrameMesh && SM != DoorMesh)
			{
				PanelMeshes.Add(SM);
			}
		}

		if (PanelMeshes.Num() >= 2)
		{
			PanelMeshes.Sort([](const UStaticMeshComponent& A, const UStaticMeshComponent& B) {
				return A.GetRelativeLocation().Y < B.GetRelativeLocation().Y;
			});
			LeftDoorComponent = PanelMeshes[0];
			RightDoorComponent = PanelMeshes[PanelMeshes.Num() - 1];
		}
	}

	// Record initial locations and calculate open target locations for sliding doors
	if (LeftDoorComponent)
	{
		LeftDoorComponent->SetMobility(EComponentMobility::Movable);
		LeftClosedLoc = LeftDoorComponent->GetRelativeLocation();
	}
	if (RightDoorComponent)
	{
		RightDoorComponent->SetMobility(EComponentMobility::Movable);
		RightClosedLoc = RightDoorComponent->GetRelativeLocation();
	}

	if (LeftDoorComponent && RightDoorComponent)
	{
		// Use local sideways lateral axis (0, 1, 0) for sliding door panels in UE frames
		FVector Dir = FVector(0.0f, 1.0f, 0.0f);
		if (!SlideDirection.IsNearlyZero())
		{
			Dir = SlideDirection.GetSafeNormal();
		}

		float Dist = (SlideDistance > 10.0f) ? SlideDistance : 150.0f;
		LeftOpenLoc = LeftClosedLoc - (Dir * Dist);
		RightOpenLoc = RightClosedLoc + (Dir * Dist);
	}
	else
	{
		FVector Dir = SlideDirection.IsNearlyZero() ? FVector(0.0f, 1.0f, 0.0f) : SlideDirection.GetSafeNormal();
		float Dist = (SlideDistance > 10.0f) ? SlideDistance : 150.0f;

		if (LeftDoorComponent) LeftOpenLoc = LeftClosedLoc - (Dir * Dist);
		if (RightDoorComponent) RightOpenLoc = RightClosedLoc + (Dir * Dist);
	}

	// Single swinging door fallback
	if (DoorMesh)
	{
		ClosedRotation = DoorMesh->GetRelativeRotation();
		OpenRotation = ClosedRotation + FRotator(0.0f, OpenYawAngle, 0.0f);
	}
}

void AMathTrapExitDoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsOpening)
	{
		return;
	}

	bool bLeftFinished = true;
	bool bRightFinished = true;
	bool bSingleFinished = true;

	// Slide Left Door Panel
	if (LeftDoorComponent)
	{
		FVector CurrentLoc = LeftDoorComponent->GetRelativeLocation();
		FVector NewLoc = FMath::VInterpTo(CurrentLoc, LeftOpenLoc, DeltaTime, OpenSpeed);
		LeftDoorComponent->SetRelativeLocation(NewLoc);

		if (FVector::DistSquared(NewLoc, LeftOpenLoc) > 1.0f)
		{
			bLeftFinished = false;
		}
	}

	// Slide Right Door Panel
	if (RightDoorComponent)
	{
		FVector CurrentLoc = RightDoorComponent->GetRelativeLocation();
		FVector NewLoc = FMath::VInterpTo(CurrentLoc, RightOpenLoc, DeltaTime, OpenSpeed);
		RightDoorComponent->SetRelativeLocation(NewLoc);

		if (FVector::DistSquared(NewLoc, RightOpenLoc) > 1.0f)
		{
			bRightFinished = false;
		}
	}

	// Swing Single Door Fallback
	if (DoorMesh)
	{
		FRotator CurrentRot = DoorMesh->GetRelativeRotation();
		FRotator NewRot = FMath::RInterpTo(CurrentRot, OpenRotation, DeltaTime, OpenSpeed);
		DoorMesh->SetRelativeRotation(NewRot);

		if (!NewRot.Equals(OpenRotation, 0.5f))
		{
			bSingleFinished = false;
		}
	}

	if (bLeftFinished && bRightFinished && bSingleFinished)
	{
		if (LeftDoorComponent) LeftDoorComponent->SetRelativeLocation(LeftOpenLoc);
		if (RightDoorComponent) RightDoorComponent->SetRelativeLocation(RightOpenLoc);
		if (DoorMesh) DoorMesh->SetRelativeRotation(OpenRotation);

		bIsOpening = false;
		bIsOpen = true;
	}
}

void AMathTrapExitDoor::UnlockAndOpen()
{
	if (bIsOpen)
	{
		return;
	}

	bIsOpening = true;

	// Disable collision on all door primitive components when unlocked
	TArray<UPrimitiveComponent*> Prims;
	GetComponents<UPrimitiveComponent>(Prims);
	for (UPrimitiveComponent* Prim : Prims)
	{
		if (Prim)
		{
			Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	if (StatusText)
	{
		StatusText->SetText(FText::FromString(TEXT("UNLOCKED")));
		StatusText->SetTextRenderColor(FColor(40, 255, 80, 255)); // Neon green
	}

	if (UnlockSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, UnlockSound, GetActorLocation());
	}
}

void AMathTrapExitDoor::ResetDoor()
{
	bIsOpen = false;
	bIsOpening = false;

	// Restore collision on door components
	TArray<UPrimitiveComponent*> Prims;
	GetComponents<UPrimitiveComponent>(Prims);
	for (UPrimitiveComponent* Prim : Prims)
	{
		if (Prim)
		{
			Prim->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
	}

	if (LeftDoorComponent)
	{
		LeftDoorComponent->SetRelativeLocation(LeftClosedLoc);
	}

	if (RightDoorComponent)
	{
		RightDoorComponent->SetRelativeLocation(RightClosedLoc);
	}

	if (DoorMesh)
	{
		DoorMesh->SetRelativeRotation(ClosedRotation);
	}

	if (StatusText)
	{
		StatusText->SetText(FText::FromString(TEXT("LOCKED")));
		StatusText->SetTextRenderColor(FColor(255, 40, 40, 255));
	}
}
