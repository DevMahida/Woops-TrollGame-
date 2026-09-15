// Copyright Epic Games, Inc. All Rights Reserved.

#include "MathTrapExitDoor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/GameplayStatics.h"

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

	if (DoorMesh)
	{
		ClosedRotation = DoorMesh->GetRelativeRotation();
		OpenRotation = ClosedRotation + FRotator(0.0f, OpenYawAngle, 0.0f);
	}
}

void AMathTrapExitDoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsOpening || !DoorMesh)
	{
		return;
	}

	FRotator CurrentRot = DoorMesh->GetRelativeRotation();
	FRotator NewRot = FMath::RInterpTo(CurrentRot, OpenRotation, DeltaTime, OpenSpeed);
	DoorMesh->SetRelativeRotation(NewRot);

	if (NewRot.Equals(OpenRotation, 0.5f))
	{
		DoorMesh->SetRelativeRotation(OpenRotation);
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
