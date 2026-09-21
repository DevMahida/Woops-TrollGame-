// Copyright Epic Games, Inc. All Rights Reserved.

#include "MathTrapManager.h"
#include "MathTrapBuzzer.h"
#include "MathTrapExitDoor.h"
#include "TrollGameCharacter.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "TimerManager.h"

AMathTrapManager::AMathTrapManager()
{
	PrimaryActorTick.bCanEverTick = true;

	CurrentAttempt = 1;
	ConsequenceType = EMathTrapConsequence::TeleportToStart;
	LaunchForce = 1200.0f;

	InitializeDefaultEquations();
}

void AMathTrapManager::InitializeDefaultEquations()
{
	EquationPool.Empty();

	// Answer = 1
	EquationPool.Add(FComplexMathEquation(TEXT("(6 * 2 / 4 + 5 - 7)"), 1));
	EquationPool.Add(FComplexMathEquation(TEXT("(18 / (3 * 2) + 2^2 - 6)"), 1));
	EquationPool.Add(FComplexMathEquation(TEXT("(2^3 / 4 + sqrt(9) - 4)"), 1));
	EquationPool.Add(FComplexMathEquation(TEXT("(10 / 2 - 2^2)"), 1));

	// Answer = 2
	EquationPool.Add(FComplexMathEquation(TEXT("(8 / 2 * (1 + 3) - 14)"), 2));
	EquationPool.Add(FComplexMathEquation(TEXT("(12 - 3 * 2 + 4 / 2 - 6)"), 2));
	EquationPool.Add(FComplexMathEquation(TEXT("(sqrt(64) / 4 + 5 * 0)"), 2));
	EquationPool.Add(FComplexMathEquation(TEXT("(sqrt(100) / 2 - 2^2 + 1)"), 2));

	// Answer = 3
	EquationPool.Add(FComplexMathEquation(TEXT("(15 / 3 + 2 * 3 - 8)"), 3));
	EquationPool.Add(FComplexMathEquation(TEXT("(3^2 - sqrt(16) - 2)"), 3));
	EquationPool.Add(FComplexMathEquation(TEXT("(2^3 - sqrt(25))"), 3));

	// Answer = 4
	EquationPool.Add(FComplexMathEquation(TEXT("(20 / 4 * 2 - 3 * 2)"), 4));
	EquationPool.Add(FComplexMathEquation(TEXT("(sqrt(81) / 3 + 4^0)"), 4));
	EquationPool.Add(FComplexMathEquation(TEXT("(2^3 / 2)"), 4));

	// Answer = 5
	EquationPool.Add(FComplexMathEquation(TEXT("(sqrt(25) + 6 / 2 - 3)"), 5));
	EquationPool.Add(FComplexMathEquation(TEXT("(10 / 2 * (3 - 2))"), 5));
	EquationPool.Add(FComplexMathEquation(TEXT("(3^2 - sqrt(16))"), 5));
	EquationPool.Add(FComplexMathEquation(TEXT("(2^2 + 1^5)"), 5));

	// Answer = 6
	EquationPool.Add(FComplexMathEquation(TEXT("(sqrt(36) * 2 / 2)"), 6));
	EquationPool.Add(FComplexMathEquation(TEXT("(sqrt(144) / 2)"), 6));
	EquationPool.Add(FComplexMathEquation(TEXT("(2^3 - sqrt(4))"), 6));
	EquationPool.Add(FComplexMathEquation(TEXT("(3 * 4 / 2)"), 6));

	// Answer = 7
	EquationPool.Add(FComplexMathEquation(TEXT("(sqrt(49) + 3 * 0)"), 7));
	EquationPool.Add(FComplexMathEquation(TEXT("(5 * 2 - sqrt(9))"), 7));
	EquationPool.Add(FComplexMathEquation(TEXT("(2^3 - 1^4)"), 7));
	EquationPool.Add(FComplexMathEquation(TEXT("(14 / 2 * (2 - 1))"), 7));

	// Answer = 8
	EquationPool.Add(FComplexMathEquation(TEXT("(sqrt(64) + 4 / 2 - 2)"), 8));
	EquationPool.Add(FComplexMathEquation(TEXT("(2^3 * (4 - 3))"), 8));
	EquationPool.Add(FComplexMathEquation(TEXT("(4 * 4 / 2)"), 8));
	EquationPool.Add(FComplexMathEquation(TEXT("(3^2 - 1^3)"), 8));

	// Answer = 9
	EquationPool.Add(FComplexMathEquation(TEXT("(3^2 + 7 * 0)"), 9));
	EquationPool.Add(FComplexMathEquation(TEXT("(5 * 2 - 1^5)"), 9));
	EquationPool.Add(FComplexMathEquation(TEXT("(sqrt(81) * (5 - 4))"), 9));
	EquationPool.Add(FComplexMathEquation(TEXT("(2^3 + 4 / 4)"), 9));

	// Answer = 10
	EquationPool.Add(FComplexMathEquation(TEXT("(sqrt(100) + 8 * 0)"), 10));
	EquationPool.Add(FComplexMathEquation(TEXT("(2 * 5 + 3 - 3)"), 10));
	EquationPool.Add(FComplexMathEquation(TEXT("(4^2 - 3 * 2)"), 10));
	EquationPool.Add(FComplexMathEquation(TEXT("(2^3 + sqrt(4))"), 10));
}

void AMathTrapManager::BeginPlay()
{
	Super::BeginPlay();

	SetupRoom();
	ResetRoom();
}

void AMathTrapManager::SetupRoom()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// ========================================================================
	// 1. Discover dedicated AMathTrapBuzzer actors
	// ========================================================================
	RoomBuzzers.Empty();
	for (TActorIterator<AMathTrapBuzzer> It(World); It; ++It)
	{
		AMathTrapBuzzer* Buzzer = *It;
		if (IsValid(Buzzer))
		{
			Buzzer->TrapManager = this;
			RoomBuzzers.Add(Buzzer);
		}
	}
	RoomBuzzers.Sort([](const AMathTrapBuzzer& A, const AMathTrapBuzzer& B) {
		return A.BuzzerNumber < B.BuzzerNumber;
	});

	// ========================================================================
	// 2. Discover generic buzzer actors (level-placed actors with "Buzzer" in name/label/tag)
	// ========================================================================
	GenericBuzzers.Empty();

	// Collect candidate actors - only root actors (not attached children)
	TArray<AActor*> Candidates;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor)) continue;
		if (Actor == this) continue;
		if (Actor->IsA<AMathTrapBuzzer>()) continue;
		if (Actor->IsA<ACharacter>()) continue;
		if (Actor == RoomScreenActor) continue;
		if (Actor->GetAttachParentActor() != nullptr) continue;

		FString Label = Actor->GetActorLabel();
		FString Name = Actor->GetName();

		bool bMatch = Label.Contains(TEXT("Buzzer"), ESearchCase::IgnoreCase)
		           || Name.Contains(TEXT("Buzzer"), ESearchCase::IgnoreCase)
		           || Actor->ActorHasTag(TEXT("Buzzer"));

		if (bMatch)
		{
			Candidates.Add(Actor);
		}
	}

	// Deduplicate actors at the same location (within 40 units)
	TArray<AActor*> UniquePedestals;
	for (AActor* Cand : Candidates)
	{
		if (!IsValid(Cand)) continue;
		FVector CandLoc = Cand->GetActorLocation();
		bool bDupe = false;
		for (AActor* Existing : UniquePedestals)
		{
			if (IsValid(Existing) && FVector::Dist2D(CandLoc, Existing->GetActorLocation()) < 40.0f)
			{
				bDupe = true;
				break;
			}
		}
		if (!bDupe)
		{
			UniquePedestals.Add(Cand);
		}
	}

	// Sort spatially along the room's primary axis (whichever axis has most spread)
	if (UniquePedestals.Num() > 1)
	{
		// Determine which axis (X or Y) has the most spread
		float MinX = MAX_FLT, MaxX = -MAX_FLT, MinY = MAX_FLT, MaxY = -MAX_FLT;
		for (AActor* A : UniquePedestals)
		{
			FVector L = A->GetActorLocation();
			MinX = FMath::Min(MinX, L.X);
			MaxX = FMath::Max(MaxX, L.X);
			MinY = FMath::Min(MinY, L.Y);
			MaxY = FMath::Max(MaxY, L.Y);
		}

		if (FMath::Abs(MaxX - MinX) >= FMath::Abs(MaxY - MinY))
		{
			UniquePedestals.Sort([](const AActor& A, const AActor& B) {
				return A.GetActorLocation().X < B.GetActorLocation().X;
			});
		}
		else
		{
			UniquePedestals.Sort([](const AActor& A, const AActor& B) {
				return A.GetActorLocation().Y < B.GetActorLocation().Y;
			});
		}
	}

	// Determine maximum top Z height across ALL buzzers in the room so all number labels sit at the exact same level
	float MaxRoomTopZ = -999999.0f;
	for (AActor* Actor : UniquePedestals)
	{
		if (!IsValid(Actor)) continue;
		MaxRoomTopZ = FMath::Max(MaxRoomTopZ, Actor->GetActorLocation().Z + 110.0f);

		TArray<UStaticMeshComponent*> Meshes;
		Actor->GetComponents<UStaticMeshComponent>(Meshes);
		for (UStaticMeshComponent* SM : Meshes)
		{
			if (IsValid(SM))
			{
				FBoxSphereBounds SMOB = SM->CalcBounds(SM->GetComponentTransform());
				MaxRoomTopZ = FMath::Max(MaxRoomTopZ, SMOB.Origin.Z + SMOB.BoxExtent.Z + 60.0f);
			}
		}
	}

	// Build GenericBuzzerInfo for each discovered pedestal
	for (int32 i = 0; i < UniquePedestals.Num(); ++i)
	{
		AActor* Actor = UniquePedestals[i];
		if (!IsValid(Actor)) continue;

		FGenericBuzzerInfo Info;
		Info.Number = i + 1;
		Info.BuzzerActor = Actor;

		// Find the button component (highest static mesh, or fallback to root)
		USceneComponent* RootComp = Actor->GetRootComponent();
		UStaticMeshComponent* HighestMesh = nullptr;
		float HighestZ = -999999.0f;

		TArray<UStaticMeshComponent*> Meshes;
		Actor->GetComponents<UStaticMeshComponent>(Meshes);
		for (UStaticMeshComponent* SM : Meshes)
		{
			if (IsValid(SM) && SM->GetComponentLocation().Z > HighestZ)
			{
				HighestZ = SM->GetComponentLocation().Z;
				HighestMesh = SM;
			}
		}

		Info.ButtonComponent = HighestMesh ? Cast<USceneComponent>(HighestMesh) : RootComp;

		if (Info.ButtonComponent)
		{
			Info.InitialRelLoc = Info.ButtonComponent->GetRelativeLocation();
			Info.TargetRelLoc = Info.InitialRelLoc - FVector(0.0f, 0.0f, 6.0f);
		}

		// Create the number text label above the buzzer at uniform Z level
		if (RootComp)
		{
			FVector ActorLoc = Actor->GetActorLocation();
			FVector LabelLoc = FVector(ActorLoc.X, ActorLoc.Y, MaxRoomTopZ);

			UTextRenderComponent* TextComp = NewObject<UTextRenderComponent>(Actor, *FString::Printf(TEXT("BuzzerLabel_%d"), Info.Number));
			if (TextComp)
			{
				TextComp->RegisterComponent();
				TextComp->AttachToComponent(RootComp, FAttachmentTransformRules::KeepWorldTransform);
				TextComp->SetWorldLocation(LabelLoc);
				TextComp->SetWorldScale3D(FVector(1.0f));
				TextComp->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
				TextComp->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
				TextComp->SetWorldSize(40.0f);
				TextComp->SetTextRenderColor(FColor(255, 30, 30, 255));
				TextComp->SetText(FText::AsNumber(Info.Number));
				TextComp->SetVisibility(true);
				Info.NumberText = TextComp;
			}

			// Create highlight light (hidden by default, used for attempt 5 prank)
			FVector LightLoc = FVector(ActorLoc.X, ActorLoc.Y, ActorLoc.Z + 30.0f);
			UPointLightComponent* LightComp = NewObject<UPointLightComponent>(Actor, *FString::Printf(TEXT("HighlightLight_%d"), Info.Number));
			if (LightComp)
			{
				LightComp->RegisterComponent();
				LightComp->AttachToComponent(RootComp, FAttachmentTransformRules::KeepWorldTransform);
				LightComp->SetWorldLocation(LightLoc);
				LightComp->SetLightColor(FLinearColor(0.05f, 1.0f, 0.2f));
				LightComp->SetIntensity(4000.0f);
				LightComp->SetAttenuationRadius(300.0f);
				LightComp->SetVisibility(false);
				Info.HighlightLight = LightComp;
			}
		}

		GenericBuzzers.Add(Info);
	}

	GenericBuzzers.Sort([](const FGenericBuzzerInfo& A, const FGenericBuzzerInfo& B) {
		return A.Number < B.Number;
	});

	// ========================================================================
	// 3. Auto-locate exit door & left/right door panels in the world
	// ========================================================================
	if (!ExitDoor)
	{
		for (TActorIterator<AMathTrapExitDoor> It(World); It; ++It)
		{
			ExitDoor = *It;
			break;
		}
	}

	LeftDoorComp = nullptr;
	RightDoorComp = nullptr;

	// Priority 1: Check components inside any actor in the world matching Level_1_Door_1_left / Level_1_Door_1_right or Door_left / Door_right
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor) || Actor == this || Actor->IsA<ACharacter>()) continue;

		FString Label = Actor->GetActorLabel();
		FString Name = Actor->GetName();

		if (Label.Contains(TEXT("Screen"), ESearchCase::IgnoreCase) || Label.Contains(TEXT("Buzzer"), ESearchCase::IgnoreCase)) continue;

		TArray<USceneComponent*> Comps;
		Actor->GetComponents<USceneComponent>(Comps);
		for (USceneComponent* Comp : Comps)
		{
			if (!Comp) continue;
			FString CName = Comp->GetName();

			if (!LeftDoorComp)
			{
				if (CName.Equals(TEXT("Level_1_Door_1_left"), ESearchCase::IgnoreCase) ||
					CName.Contains(TEXT("Door_1_left"), ESearchCase::IgnoreCase) ||
					CName.Contains(TEXT("Door_left"), ESearchCase::IgnoreCase) ||
					(CName.Contains(TEXT("left"), ESearchCase::IgnoreCase) && (Label.Contains(TEXT("Door"), ESearchCase::IgnoreCase) || Name.Contains(TEXT("Door"), ESearchCase::IgnoreCase))))
				{
					LeftDoorComp = Comp;
				}
			}

			if (!RightDoorComp)
			{
				if (CName.Equals(TEXT("Level_1_Door_1_right"), ESearchCase::IgnoreCase) ||
					CName.Contains(TEXT("Door_1_right"), ESearchCase::IgnoreCase) ||
					CName.Contains(TEXT("Door_right"), ESearchCase::IgnoreCase) ||
					(CName.Contains(TEXT("right"), ESearchCase::IgnoreCase) && (Label.Contains(TEXT("Door"), ESearchCase::IgnoreCase) || Name.Contains(TEXT("Door"), ESearchCase::IgnoreCase))))
				{
					RightDoorComp = Comp;
				}
			}
		}
	}

	// Priority 2: Check actor root components where the actor label or name itself contains Door AND left/right (e.g. Level_1_Door_1_left)
	if (!LeftDoorComp || !RightDoorComp)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (!IsValid(Actor) || Actor == this || Actor->IsA<ACharacter>()) continue;

			FString Label = Actor->GetActorLabel();
			FString Name = Actor->GetName();

			if (Label.Contains(TEXT("Screen"), ESearchCase::IgnoreCase) || Label.Contains(TEXT("Buzzer"), ESearchCase::IgnoreCase)) continue;

			// Check for left door actor
			if (!LeftDoorComp)
			{
				bool bIsLeftDoor = (Label.Equals(TEXT("Level_1_Door_1_left"), ESearchCase::IgnoreCase) || Name.Equals(TEXT("Level_1_Door_1_left"), ESearchCase::IgnoreCase) ||
					Label.Contains(TEXT("Door_1_left"), ESearchCase::IgnoreCase) || Name.Contains(TEXT("Door_1_left"), ESearchCase::IgnoreCase) ||
					Label.Contains(TEXT("Door_left"), ESearchCase::IgnoreCase) || Name.Contains(TEXT("Door_left"), ESearchCase::IgnoreCase) ||
					((Label.Contains(TEXT("Door"), ESearchCase::IgnoreCase) || Name.Contains(TEXT("Door"), ESearchCase::IgnoreCase)) && (Label.Contains(TEXT("left"), ESearchCase::IgnoreCase) || Name.Contains(TEXT("left"), ESearchCase::IgnoreCase))));
				if (bIsLeftDoor)
				{
					LeftDoorComp = Actor->GetRootComponent();
				}
			}

			// Check for right door actor
			if (!RightDoorComp)
			{
				bool bIsRightDoor = (Label.Equals(TEXT("Level_1_Door_1_right"), ESearchCase::IgnoreCase) || Name.Equals(TEXT("Level_1_Door_1_right"), ESearchCase::IgnoreCase) ||
					Label.Contains(TEXT("Door_1_right"), ESearchCase::IgnoreCase) || Name.Contains(TEXT("Door_1_right"), ESearchCase::IgnoreCase) ||
					Label.Contains(TEXT("Door_right"), ESearchCase::IgnoreCase) || Name.Contains(TEXT("Door_right"), ESearchCase::IgnoreCase) ||
					((Label.Contains(TEXT("Door"), ESearchCase::IgnoreCase) || Name.Contains(TEXT("Door"), ESearchCase::IgnoreCase)) && (Label.Contains(TEXT("right"), ESearchCase::IgnoreCase) || Name.Contains(TEXT("right"), ESearchCase::IgnoreCase))));
				if (bIsRightDoor)
				{
					RightDoorComp = Actor->GetRootComponent();
				}
			}
		}
	}

	// Priority 3: Fallback search across all actors for any actor containing Level_1_Door_1 or Door_1
	if (!LeftDoorComp || !RightDoorComp)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (!IsValid(Actor) || Actor == this || Actor->IsA<ACharacter>()) continue;

			FString Label = Actor->GetActorLabel();
			FString Name = Actor->GetName();

			if (!LeftDoorComp && (Label.Contains(TEXT("left"), ESearchCase::IgnoreCase) || Name.Contains(TEXT("left"), ESearchCase::IgnoreCase)) && (Label.Contains(TEXT("Level_1_Door"), ESearchCase::IgnoreCase) || Name.Contains(TEXT("Level_1_Door"), ESearchCase::IgnoreCase)))
			{
				LeftDoorComp = Actor->GetRootComponent();
			}
			if (!RightDoorComp && (Label.Contains(TEXT("right"), ESearchCase::IgnoreCase) || Name.Contains(TEXT("right"), ESearchCase::IgnoreCase)) && (Label.Contains(TEXT("Level_1_Door"), ESearchCase::IgnoreCase) || Name.Contains(TEXT("Level_1_Door"), ESearchCase::IgnoreCase)))
			{
				RightDoorComp = Actor->GetRootComponent();
			}
		}
	}

	if (LeftDoorComp && RightDoorComp)
	{
		LeftDoorComp->SetMobility(EComponentMobility::Movable);
		RightDoorComp->SetMobility(EComponentMobility::Movable);

		LeftDoorClosedLoc = LeftDoorComp->GetComponentLocation();
		RightDoorClosedLoc = RightDoorComp->GetComponentLocation();

		// Use the door's RightVector (sideways lateral axis along the door frame wall)
		FVector SlideDir = LeftDoorComp->GetRightVector();
		SlideDir.Z = 0.0f;
		if (SlideDir.IsNearlyZero())
		{
			SlideDir = FVector(0.0f, 1.0f, 0.0f);
		}
		else
		{
			SlideDir.Normalize();
		}

		LeftDoorOpenLoc = LeftDoorClosedLoc - (SlideDir * 180.0f);
		RightDoorOpenLoc = RightDoorClosedLoc + (SlideDir * 180.0f);

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green,
				FString::Printf(TEXT("MathTrap Door Setup SUCCESS: Left='%s' (%s), Right='%s' (%s) | SlideDir=(%.1f, %.1f, %.1f)"),
				*LeftDoorComp->GetName(), LeftDoorComp->GetOwner() ? *LeftDoorComp->GetOwner()->GetActorLabel() : TEXT("NoOwner"),
				*RightDoorComp->GetName(), RightDoorComp->GetOwner() ? *RightDoorComp->GetOwner()->GetActorLabel() : TEXT("NoOwner"),
				SlideDir.X, SlideDir.Y, SlideDir.Z));
		}
	}
	else if (ExitDoor)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green,
				TEXT("MathTrap Door Setup SUCCESS: Using AMathTrapExitDoor actor."));
		}
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red,
				FString::Printf(TEXT("MathTrap Door Setup WARNING: Could not find left/right door targets! Found Left='%s', Right='%s'"),
				LeftDoorComp ? *LeftDoorComp->GetName() : TEXT("NULL"),
				RightDoorComp ? *RightDoorComp->GetName() : TEXT("NULL")));
		}
	}

	// ========================================================================
	// 4. Auto-locate Screen2 (room screen actor)
	// ========================================================================
	if (!RoomScreenActor)
	{
		// First try to find "Screen2" specifically
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (!IsValid(Actor)) continue;
			FString Label = Actor->GetActorLabel();
			FString Name = Actor->GetName();

			if (Label.Equals(TEXT("Screen2"), ESearchCase::IgnoreCase)
			 || Label.Equals(TEXT("Screen_2"), ESearchCase::IgnoreCase)
			 || Actor->ActorHasTag(TEXT("Screen2"))
			 || Name.Contains(TEXT("Screen2"), ESearchCase::IgnoreCase))
			{
				RoomScreenActor = Actor;
				break;
			}
		}

		// Fallback to any actor named "Screen"
		if (!RoomScreenActor)
		{
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				AActor* Actor = *It;
				if (!IsValid(Actor)) continue;
				FString Label = Actor->GetActorLabel();
				if (Label.Equals(TEXT("Screen"), ESearchCase::IgnoreCase) || Actor->ActorHasTag(TEXT("Screen")))
				{
					RoomScreenActor = Actor;
					break;
				}
			}
		}
	}

	// ========================================================================
	// 5. Auto-locate room start point
	// ========================================================================
	if (!RoomStartPoint)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (!IsValid(Actor)) continue;
			FString Label = Actor->GetActorLabel();
			FString Name = Actor->GetName();

			// Ignore checkpoint_2 or post-math room checkpoints when finding room entrance start point
			if (Label.Contains(TEXT("CheckPoint_2"), ESearchCase::IgnoreCase) || 
			    Name.Contains(TEXT("CheckPoint_2"), ESearchCase::IgnoreCase) ||
			    Label.Contains(TEXT("CheckPoint_2"), ESearchCase::IgnoreCase) || 
			    Name.Contains(TEXT("CheckPoint_2"), ESearchCase::IgnoreCase) ||
			    Label.Contains(TEXT("CheckPoint2"), ESearchCase::IgnoreCase) || 
			    Name.Contains(TEXT("CheckPoint2"), ESearchCase::IgnoreCase))
			{
				continue;
			}

			if (Label.Contains(TEXT("MathRoomStart"), ESearchCase::IgnoreCase)
			 || Label.Contains(TEXT("MathStart"), ESearchCase::IgnoreCase)
			 || Label.Equals(TEXT("Start2"), ESearchCase::IgnoreCase)
			 || Label.Contains(TEXT("CheckPoint_1"), ESearchCase::IgnoreCase)
			 || Label.Contains(TEXT("CheckPoint"), ESearchCase::IgnoreCase))
			{
				RoomStartPoint = Actor;
				break;
			}
		}
	}

	// ========================================================================
	// 6. Setup 3D Text on RoomScreenActor
	// ========================================================================
	if (RoomScreenActor)
	{
		ScreenTextComp = RoomScreenActor->FindComponentByClass<UTextRenderComponent>();
		if (!ScreenTextComp)
		{
			ScreenTextComp = NewObject<UTextRenderComponent>(RoomScreenActor, TEXT("MathRoomScreenText"));
			if (ScreenTextComp)
			{
				ScreenTextComp->RegisterComponent();
				ScreenTextComp->AttachToComponent(RoomScreenActor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
			}
		}

		if (ScreenTextComp)
		{
			ScreenTextComp->SetRelativeLocation(FVector(0.0f, 56.0f, 0.0f));
			ScreenTextComp->SetRelativeRotation(FRotator(0.0f, 90.0f, 180.0f));
			ScreenTextComp->SetWorldScale3D(FVector(1.0f));
			ScreenTextComp->SetWorldSize(65.0f);
			ScreenTextComp->SetHorizSpacingAdjust(1.0f);
			ScreenTextComp->SetVertSpacingAdjust(0.90f);
			ScreenTextComp->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
			ScreenTextComp->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
		}
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Cyan,
			FString::Printf(TEXT("MathTrap Setup: Screen='%s', GenericBuzzers=%d, DedicatedBuzzers=%d"),
			RoomScreenActor ? *RoomScreenActor->GetActorLabel() : TEXT("NONE"),
			GenericBuzzers.Num(), RoomBuzzers.Num()));
	}
}

void AMathTrapManager::ActivateTrap()
{
	if (bTrapActivated) return;

	bTrapActivated = true;
	CurrentAttempt = 1;

	PickNewEquation();
	ShuffleBuzzerNumbers();

	FString WelcomeText = FString::Printf(
		TEXT("DOOR LOCKED: SOLVE TO ESCAPE\n\n%s = ?\n\nPress the matching numbered buzzer (1 - 10)!"),
		*CurrentEquation.EquationText
	);

	UpdateScreenText(WelcomeText, FColor(0, 255, 235, 255));
}

void AMathTrapManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ACharacter* PlayerChar = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (!PlayerChar) return;

	FVector PlayerLoc = PlayerChar->GetActorLocation();
	UWorld* World = GetWorld();
	if (!World) return;
	float WorldTime = World->GetTimeSeconds();

	// Animate Door Sliding Open in World Space
	if (bDoorOpeningAnimation)
	{
		if (LeftDoorComp)
		{
			FVector Cur = LeftDoorComp->GetComponentLocation();
			FVector NewL = FMath::VInterpTo(Cur, LeftDoorOpenLoc, DeltaTime, 3.0f);
			LeftDoorComp->SetWorldLocation(NewL);
		}
		if (RightDoorComp)
		{
			FVector Cur = RightDoorComp->GetComponentLocation();
			FVector NewL = FMath::VInterpTo(Cur, RightDoorOpenLoc, DeltaTime, 3.0f);
			RightDoorComp->SetWorldLocation(NewL);
		}
	}

	// 1. Billboard: rotate all number labels to face the player
	for (int32 i = 0; i < GenericBuzzers.Num(); ++i)
	{
		UTextRenderComponent* TextComp = GenericBuzzers[i].NumberText;
		if (!TextComp) continue;

		FVector TextLoc = TextComp->GetComponentLocation();
		FVector DirToPlayer = PlayerLoc - TextLoc;
		DirToPlayer.Z = 0.0f;

		if (!DirToPlayer.IsNearlyZero())
		{
			FRotator LookRot = DirToPlayer.Rotation();
			LookRot.Pitch = 0.0f;
			LookRot.Roll = 0.0f;
			TextComp->SetWorldRotation(LookRot);
		}
	}

	// 2. Check checkpoint activation
	if (!bTrapActivated)
	{
		ATrollGameCharacter* TrollChar = Cast<ATrollGameCharacter>(PlayerChar);
		if (TrollChar && TrollChar->HasActiveCheckpoint())
		{
			ActivateTrap();
		}
		else
		{
			return;
		}
	}

	// 3. Animate button depression
	for (int32 i = 0; i < GenericBuzzers.Num(); ++i)
	{
		FGenericBuzzerInfo& Buzzer = GenericBuzzers[i];
		if (!IsValid(Buzzer.BuzzerActor)) continue;

		if (Buzzer.bIsAnimating && Buzzer.ButtonComponent)
		{
			FVector CurrentRelLoc = Buzzer.ButtonComponent->GetRelativeLocation();

			if (!Buzzer.bIsReturning)
			{
				FVector NewLoc = FMath::VInterpTo(CurrentRelLoc, Buzzer.TargetRelLoc, DeltaTime, 15.0f);
				Buzzer.ButtonComponent->SetRelativeLocation(NewLoc);

				if (FVector::DistSquared(NewLoc, Buzzer.TargetRelLoc) < 0.25f)
				{
					Buzzer.bIsReturning = true;
				}
			}
			else
			{
				FVector NewLoc = FMath::VInterpTo(CurrentRelLoc, Buzzer.InitialRelLoc, DeltaTime, 15.0f);
				Buzzer.ButtonComponent->SetRelativeLocation(NewLoc);

				if (FVector::DistSquared(NewLoc, Buzzer.InitialRelLoc) < 0.25f)
				{
					Buzzer.ButtonComponent->SetRelativeLocation(Buzzer.InitialRelLoc);
					Buzzer.bIsAnimating = false;
					Buzzer.bIsReturning = false;
				}
			}
		}
	}

	// 4. Look-At Interaction (Player must look at buzzer and press [E])
	if (bTrapCompleted || (ExitDoor && ExitDoor->bIsOpen))
	{
		if (CenterPromptTextComp)
		{
			CenterPromptTextComp->SetVisibility(false);
		}
		return;
	}

	APlayerController* PC = Cast<APlayerController>(PlayerChar->GetController());
	if (!PC) return;

	FVector CameraLoc;
	FRotator CameraRot;
	PC->GetPlayerViewPoint(CameraLoc, CameraRot);
	FVector CameraForward = CameraRot.Vector();

	FGenericBuzzerInfo* FocusedGenericBuzzer = nullptr;
	AMathTrapBuzzer* FocusedDedicatedBuzzer = nullptr;
	float BestDot = 0.90f; // Tight ~25 degrees view cone

	// Direct Line Trace check from camera
	FHitResult HitResult;
	FVector TraceEnd = CameraLoc + (CameraForward * 350.0f);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(PlayerChar);

	AActor* HitActor = nullptr;
	if (World->LineTraceSingleByChannel(HitResult, CameraLoc, TraceEnd, ECC_Visibility, QueryParams))
	{
		HitActor = HitResult.GetActor();
	}

	// Check generic level buzzers
	for (int32 i = 0; i < GenericBuzzers.Num(); ++i)
	{
		FGenericBuzzerInfo& Buzzer = GenericBuzzers[i];
		if (!IsValid(Buzzer.BuzzerActor)) continue;

		if (HitActor && (HitActor == Buzzer.BuzzerActor || HitActor->IsAttachedTo(Buzzer.BuzzerActor) || Buzzer.BuzzerActor->IsAttachedTo(HitActor)))
		{
			FocusedGenericBuzzer = &Buzzer;
			break;
		}

		FVector BuzzerLoc = Buzzer.ButtonComponent
			? Buzzer.ButtonComponent->GetComponentLocation()
			: Buzzer.BuzzerActor->GetActorLocation();

		float Dist = FVector::Dist(CameraLoc, BuzzerLoc);
		if (Dist <= 350.0f)
		{
			FVector DirToBuzzer = (BuzzerLoc - CameraLoc).GetSafeNormal();
			float Dot = FVector::DotProduct(CameraForward, DirToBuzzer);

			if (Dot > BestDot)
			{
				BestDot = Dot;
				FocusedGenericBuzzer = &Buzzer;
				FocusedDedicatedBuzzer = nullptr;
			}
		}
	}

	// Check dedicated AMathTrapBuzzer actors
	if (!FocusedGenericBuzzer)
	{
		for (AMathTrapBuzzer* B : RoomBuzzers)
		{
			if (!IsValid(B)) continue;

			if (HitActor && (HitActor == B || HitActor->IsAttachedTo(B)))
			{
				FocusedDedicatedBuzzer = B;
				break;
			}

			FVector BuzzerLoc = B->GetActorLocation();
			float Dist = FVector::Dist(CameraLoc, BuzzerLoc);
			if (Dist <= 350.0f)
			{
				FVector DirToBuzzer = (BuzzerLoc - CameraLoc).GetSafeNormal();
				float Dot = FVector::DotProduct(CameraForward, DirToBuzzer);

				if (Dot > BestDot)
				{
					BestDot = Dot;
					FocusedDedicatedBuzzer = B;
				}
			}
		}
	}

	// Lazy-create center screen interaction prompt attached to camera
	if (!CenterPromptTextComp && PlayerChar)
	{
		ATrollGameCharacter* TrollChar = Cast<ATrollGameCharacter>(PlayerChar);
		USceneComponent* AttachParent = (TrollChar && TrollChar->GetFirstPersonCameraComponent())
			? Cast<USceneComponent>(TrollChar->GetFirstPersonCameraComponent())
			: PlayerChar->GetRootComponent();

		if (AttachParent)
		{
			CenterPromptTextComp = NewObject<UTextRenderComponent>(PlayerChar, TEXT("MathRoomCenterPromptText"));
			if (CenterPromptTextComp)
			{
				CenterPromptTextComp->RegisterComponent();
				CenterPromptTextComp->AttachToComponent(AttachParent, FAttachmentTransformRules::KeepRelativeTransform);
				CenterPromptTextComp->SetRelativeLocation(FVector(100.0f, 0.0f, -10.0f));
				CenterPromptTextComp->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
				CenterPromptTextComp->SetWorldScale3D(FVector(1.0f));
				CenterPromptTextComp->SetWorldSize(4.5f);
				CenterPromptTextComp->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
				CenterPromptTextComp->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
				CenterPromptTextComp->SetTextRenderColor(FColor(0, 0, 0, 255));
				CenterPromptTextComp->SetVisibility(false);
			}
		}
	}

	// Handle center screen prompt and [E] key input
	if (FocusedGenericBuzzer)
	{
		if (CenterPromptTextComp)
		{
			CenterPromptTextComp->SetText(FText::FromString(FString::Printf(TEXT("[E] PRESS BUZZER #%d"), FocusedGenericBuzzer->Number)));
			CenterPromptTextComp->SetTextRenderColor(FColor(0, 0, 0, 255));
			CenterPromptTextComp->SetVisibility(true);
		}

		if (PC->WasInputKeyJustPressed(EKeys::E))
		{
			if ((WorldTime - FocusedGenericBuzzer->LastPressedTime) > 1.0f)
			{
				FocusedGenericBuzzer->LastPressedTime = WorldTime;
				FocusedGenericBuzzer->bIsAnimating = true;
				FocusedGenericBuzzer->bIsReturning = false;
				ProcessBuzzerInput(FocusedGenericBuzzer->Number, FocusedGenericBuzzer->BuzzerActor, PlayerChar);
			}
		}
	}
	else if (FocusedDedicatedBuzzer)
	{
		if (CenterPromptTextComp)
		{
			CenterPromptTextComp->SetText(FText::FromString(FString::Printf(TEXT("[E] PRESS BUZZER #%d"), FocusedDedicatedBuzzer->BuzzerNumber)));
			CenterPromptTextComp->SetTextRenderColor(FColor(0, 0, 0, 255));
			CenterPromptTextComp->SetVisibility(true);
		}

		if (PC->WasInputKeyJustPressed(EKeys::E))
		{
			FocusedDedicatedBuzzer->PressBuzzer(PlayerChar);
		}
	}
	else
	{
		if (CenterPromptTextComp)
		{
			CenterPromptTextComp->SetVisibility(false);
		}
	}
}

void AMathTrapManager::ResetRoom()
{
	CurrentAttempt = 1;
	bTrapCompleted = false;
	bDoorOpeningAnimation = false;

	if (LeftDoorComp) LeftDoorComp->SetWorldLocation(LeftDoorClosedLoc);
	if (RightDoorComp) RightDoorComp->SetWorldLocation(RightDoorClosedLoc);

	auto EnableDoorCollision = [](AActor* DoorActor)
	{
		if (!IsValid(DoorActor)) return;
		TArray<UPrimitiveComponent*> PrimComps;
		DoorActor->GetComponents<UPrimitiveComponent>(PrimComps);
		for (UPrimitiveComponent* Prim : PrimComps)
		{
			if (Prim)
			{
				Prim->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			}
		}
	};
	if (LeftDoorComp && LeftDoorComp->GetOwner()) EnableDoorCollision(LeftDoorComp->GetOwner());
	if (RightDoorComp && RightDoorComp->GetOwner()) EnableDoorCollision(RightDoorComp->GetOwner());
	if (ExitDoor) EnableDoorCollision(ExitDoor);

	if (CenterPromptTextComp)
	{
		CenterPromptTextComp->SetVisibility(false);
	}

	// Reset dedicated buzzer highlight
	if (FakeHighlightedBuzzer)
	{
		FakeHighlightedBuzzer->SetHighlight(false);
		FakeHighlightedBuzzer = nullptr;
	}

	// Reset generic buzzer highlight
	if (FakeHighlightedGenericBuzzerIndex != INDEX_NONE && GenericBuzzers.IsValidIndex(FakeHighlightedGenericBuzzerIndex))
	{
		if (GenericBuzzers[FakeHighlightedGenericBuzzerIndex].HighlightLight)
		{
			GenericBuzzers[FakeHighlightedGenericBuzzerIndex].HighlightLight->SetVisibility(false);
		}
		if (GenericBuzzers[FakeHighlightedGenericBuzzerIndex].NumberText)
		{
			GenericBuzzers[FakeHighlightedGenericBuzzerIndex].NumberText->SetTextRenderColor(FColor(255, 30, 30, 255));
		}
		FakeHighlightedGenericBuzzerIndex = INDEX_NONE;
	}

	if (ExitDoor)
	{
		ExitDoor->ResetDoor();
	}

	if (!bTrapActivated)
	{
		FString StandbyText = TEXT("ROOM 2: MATH CHALLENGE\n\n[ STANDBY ]\n\nReach the Checkpoint to begin!");
		UpdateScreenText(StandbyText, FColor(255, 200, 0, 255));
	}
	else
	{
		PickNewEquation();
		ShuffleBuzzerNumbers();

		FString WelcomeText = FString::Printf(
			TEXT("DOOR LOCKED: SOLVE TO ESCAPE\n\n%s = ?\n\nPress the matching numbered buzzer (1 - 10)!"),
			*CurrentEquation.EquationText
		);

		UpdateScreenText(WelcomeText, FColor(0, 255, 235, 255));
	}
}

void AMathTrapManager::PickNewEquation()
{
	// Seed random generator with high-resolution CPU cycles to guarantee unique selection every Play/entry
	FMath::RandInit((int32)(FPlatformTime::Cycles() ^ FDateTime::Now().GetTicks()));

	if (EquationPool.Num() == 0)
	{
		InitializeDefaultEquations();
	}

	// Filter for equations with answer in range [1, 10]
	TArray<int32> ValidIndices;
	for (int32 i = 0; i < EquationPool.Num(); ++i)
	{
		if (EquationPool[i].BaseAnswer >= 1 && EquationPool[i].BaseAnswer <= 10)
		{
			ValidIndices.Add(i);
		}
	}

	if (ValidIndices.Num() == 0)
	{
		InitializeDefaultEquations();
		for (int32 i = 0; i < EquationPool.Num(); ++i)
		{
			if (EquationPool[i].BaseAnswer >= 1 && EquationPool[i].BaseAnswer <= 10)
			{
				ValidIndices.Add(i);
			}
		}
	}

	// Purge invalid indices
	UnusedEquationIndices.RemoveAll([this](int32 Idx) {
		return !EquationPool.IsValidIndex(Idx)
		    || EquationPool[Idx].BaseAnswer < 1
		    || EquationPool[Idx].BaseAnswer > 10;
	});

	// Refill and shuffle when exhausted
	if (UnusedEquationIndices.Num() == 0)
	{
		UnusedEquationIndices = ValidIndices;
		for (int32 i = UnusedEquationIndices.Num() - 1; i > 0; --i)
		{
			int32 SwapIdx = FMath::RandRange(0, i);
			UnusedEquationIndices.Swap(i, SwapIdx);
		}
	}

	if (UnusedEquationIndices.Num() > 0)
	{
		int32 RandPos = FMath::RandRange(0, UnusedEquationIndices.Num() - 1);
		int32 SelectedIdx = UnusedEquationIndices[RandPos];
		UnusedEquationIndices.RemoveAt(RandPos);
		CurrentEquation = EquationPool[SelectedIdx];
	}
}

void AMathTrapManager::ShuffleBuzzerNumbers()
{
	if (GenericBuzzers.Num() == 0) return;

	FMath::RandInit((int32)(FPlatformTime::Cycles() ^ FDateTime::Now().GetTicks()));

	TArray<int32> Numbers;
	for (int32 i = 1; i <= GenericBuzzers.Num(); ++i)
	{
		Numbers.Add(i);
	}

	// Fisher-Yates shuffle
	for (int32 i = Numbers.Num() - 1; i > 0; --i)
	{
		int32 SwapIdx = FMath::RandRange(0, i);
		Numbers.Swap(i, SwapIdx);
	}

	for (int32 i = 0; i < GenericBuzzers.Num(); ++i)
	{
		GenericBuzzers[i].Number = Numbers[i];
		if (GenericBuzzers[i].NumberText)
		{
			GenericBuzzers[i].NumberText->SetText(FText::AsNumber(Numbers[i]));
		}
	}
}

void AMathTrapManager::OnBuzzerPressed(AMathTrapBuzzer* PressedBuzzer, ACharacter* PlayerCharacter)
{
	if (!PressedBuzzer) return;
	ProcessBuzzerInput(PressedBuzzer->BuzzerNumber, PressedBuzzer, PlayerCharacter);
}

void AMathTrapManager::ProcessBuzzerInput(int32 PressedNum, AActor* BuzzerActor, ACharacter* PlayerCharacter)
{
	if (bTrapCompleted || (ExitDoor && ExitDoor->bIsOpen)) return;

	FVector SoundLoc = BuzzerActor ? BuzzerActor->GetActorLocation() : GetActorLocation();
	int32 TrueAnswer = FMath::Clamp(CurrentEquation.BaseAnswer, 1, 10);

	// ====== ATTEMPT 6+ ONLY: TRUE ANSWER UNLOCKS DOOR ======
	if (CurrentAttempt >= 6 && PressedNum == TrueAnswer)
	{
		bTrapCompleted = true;
		// Clear any highlights
		if (FakeHighlightedBuzzer)
		{
			FakeHighlightedBuzzer->SetHighlight(false);
			FakeHighlightedBuzzer = nullptr;
		}
		if (FakeHighlightedGenericBuzzerIndex != INDEX_NONE && GenericBuzzers.IsValidIndex(FakeHighlightedGenericBuzzerIndex))
		{
			if (GenericBuzzers[FakeHighlightedGenericBuzzerIndex].HighlightLight)
			{
				GenericBuzzers[FakeHighlightedGenericBuzzerIndex].HighlightLight->SetVisibility(false);
			}
			if (GenericBuzzers[FakeHighlightedGenericBuzzerIndex].NumberText)
			{
				GenericBuzzers[FakeHighlightedGenericBuzzerIndex].NumberText->SetTextRenderColor(FColor(255, 225, 40, 255));
			}
			FakeHighlightedGenericBuzzerIndex = INDEX_NONE;
		}

		if (VictorySound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, VictorySound, SoundLoc);
		}

		if (ExitDoor)
		{
			ExitDoor->UnlockAndOpen();
		}

		bDoorOpeningAnimation = true;

		// Disable collision on door components, frame collision, and blocking volumes so player can pass freely
		auto DisableBlockingDoorCollision = [](AActor* DoorActor)
		{
			if (!IsValid(DoorActor)) return;
			TArray<UPrimitiveComponent*> PrimComps;
			DoorActor->GetComponents<UPrimitiveComponent>(PrimComps);
			for (UPrimitiveComponent* Prim : PrimComps)
			{
				if (Prim)
				{
					Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				}
			}
		};

		if (LeftDoorComp && LeftDoorComp->GetOwner()) DisableBlockingDoorCollision(LeftDoorComp->GetOwner());
		if (RightDoorComp && RightDoorComp->GetOwner()) DisableBlockingDoorCollision(RightDoorComp->GetOwner());
		if (ExitDoor) DisableBlockingDoorCollision(ExitDoor);

		UWorld* World = GetWorld();
		if (World)
		{
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				AActor* DoorActor = *It;
				if (!IsValid(DoorActor) || DoorActor == this || DoorActor->IsA<ACharacter>()) continue;
				FString Label = DoorActor->GetActorLabel();
				FString Name = DoorActor->GetName();
				if (Label.Contains(TEXT("Door"), ESearchCase::IgnoreCase) || Name.Contains(TEXT("Door"), ESearchCase::IgnoreCase))
				{
					if (Label.Contains(TEXT("Buzzer"), ESearchCase::IgnoreCase) || Label.Contains(TEXT("Screen"), ESearchCase::IgnoreCase)) continue;
					DisableBlockingDoorCollision(DoorActor);
				}
			}
		}

		FString WinText = TEXT("LEVEL CLEARED!\n\nDOOR UNLOCKED!\n\nYou actually solved the math problem!\nProceed through the exit door!");
		UpdateScreenText(WinText, FColor(0, 255, 100, 255));
		return;
	}

	// ====== WRONG ANSWER ======
	if (WrongBuzzerSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, WrongBuzzerSound, SoundLoc);
	}

	ApplyConsequence(PlayerCharacter, BuzzerActor);

	int32 A = TrueAnswer;
	int32 SummationRaw = A + A;
	int32 TargetBuzzer2 = (SummationRaw <= 10) ? SummationRaw : (((SummationRaw - 1) % 10) + 1);
	int32 Attempt3Raw = TargetBuzzer2 + TargetBuzzer2;
	int32 TargetBuzzer3 = (Attempt3Raw <= 10) ? Attempt3Raw : (((Attempt3Raw - 1) % 10) + 1);

	switch (CurrentAttempt)
	{
	case 1:
	{
		CurrentAttempt = 2;

		FString Hint2Text = FString::Printf(
			TEXT("DID YOU REALLY THINK IT WAS THAT EASY?\n\nNo one escapes on the first try!\nA true mathematician knows you must do the SUMMATION of the answer!\n\n(e.g., If the answer is %d, then do %d + %d = %d)\n\nNow go press buzzer #%d!"),
			A, A, A, SummationRaw, TargetBuzzer2
		);

		UpdateScreenText(Hint2Text, FColor(255, 200, 0, 255));
		break;
	}

	case 2:
	{
		CurrentAttempt = 3;

		FString Hint3Text = FString::Printf(
			TEXT("DEV OOPSIE!\n\nThe developer forgot to write the entire hint!\nHere is the remaining part:\n\n\"If the answer is %d, then press the %dth buzzer from the %dth labeled buzzer!\"\n\n(Hurry, press buzzer #%d!)"),
			TargetBuzzer2, TargetBuzzer2, TargetBuzzer2, TargetBuzzer3
		);

		UpdateScreenText(Hint3Text, FColor(255, 120, 0, 255));
		break;
	}

	case 3:
	{
		CurrentAttempt = 4;

		FString RoastText = TEXT("ARE YOU SERIOUS?!\n\nEven a calculator from 1980 has better logic than you!\n\nYou're just blindly following random hints on a screen!\nStop guessing and use your brain!");

		UpdateScreenText(RoastText, FColor(255, 50, 50, 255));
		break;
	}

	case 4:
	{
		CurrentAttempt = 5;

		// Highlight a WRONG dedicated buzzer
		FakeHighlightedBuzzer = nullptr;
		for (AMathTrapBuzzer* B : RoomBuzzers)
		{
			if (B && B->BuzzerNumber != A)
			{
				FakeHighlightedBuzzer = B;
				break;
			}
		}
		if (FakeHighlightedBuzzer)
		{
			FakeHighlightedBuzzer->SetHighlight(true);
		}

		// Highlight a WRONG generic buzzer in green
		FakeHighlightedGenericBuzzerIndex = INDEX_NONE;
		for (int32 i = 0; i < GenericBuzzers.Num(); ++i)
		{
			if (GenericBuzzers[i].Number != A)
			{
				FakeHighlightedGenericBuzzerIndex = i;
				if (GenericBuzzers[i].HighlightLight)
				{
					GenericBuzzers[i].HighlightLight->SetVisibility(true);
				}
				if (GenericBuzzers[i].NumberText)
				{
					GenericBuzzers[i].NumberText->SetTextRenderColor(FColor(50, 255, 100, 255));
				}
				break;
			}
		}

		FString FakeHelpText = TEXT("DEV APOLOGY:\n\nOkay, look... the developer felt bad for you.\n\nI highlighted the correct buzzer with a bright glowing green light so you can just escape already.\n\nGo press the glowing green buzzer!");

		UpdateScreenText(FakeHelpText, FColor(50, 255, 100, 255));
		break;
	}

	case 5:
	{
		// Clear highlights
		if (FakeHighlightedBuzzer)
		{
			FakeHighlightedBuzzer->SetHighlight(false);
			FakeHighlightedBuzzer = nullptr;
		}
		if (FakeHighlightedGenericBuzzerIndex != INDEX_NONE && GenericBuzzers.IsValidIndex(FakeHighlightedGenericBuzzerIndex))
		{
			if (GenericBuzzers[FakeHighlightedGenericBuzzerIndex].HighlightLight)
			{
				GenericBuzzers[FakeHighlightedGenericBuzzerIndex].HighlightLight->SetVisibility(false);
			}
			if (GenericBuzzers[FakeHighlightedGenericBuzzerIndex].NumberText)
			{
				GenericBuzzers[FakeHighlightedGenericBuzzerIndex].NumberText->SetTextRenderColor(FColor(255, 30, 30, 255));
			}
			FakeHighlightedGenericBuzzerIndex = INDEX_NONE;
		}

		if (PrankLaughSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, PrankLaughSound, SoundLoc);
		}

		CurrentAttempt = 6;

		FString FooledYouText = FString::Printf(
			TEXT("FOOLED YOU! HAHAHA!\n\nThere are no shortcuts in \"WHOOPS!\"\n\nNow stop looking for cheats and DO MATH PROPERLY!\n\nSolve: %s = ?\nPress the TRUE answer buzzer (1 - 10) to unlock the door!"),
			*CurrentEquation.EquationText
		);

		UpdateScreenText(FooledYouText, FColor(255, 0, 200, 255));
		break;
	}

	default:
	{
		PickNewEquation();

		FString KeepTryingText = FString::Printf(
			TEXT("WRONG BUZZER!\n\nSolve: %s = ?\n\nFind the true answer (1 - 10)!"),
			*CurrentEquation.EquationText
		);

		UpdateScreenText(KeepTryingText, FColor(255, 50, 50, 255));
		break;
	}
	}
}

void AMathTrapManager::ApplyConsequence(ACharacter* PlayerChar, AActor* FromBuzzer)
{
	// Shuffle buzzer numbers after every wrong selection
	ShuffleBuzzerNumbers();

	if (!PlayerChar) return;

	switch (ConsequenceType)
	{
	case EMathTrapConsequence::TeleportToStart:
	{
		ATrollGameCharacter* TrollChar = Cast<ATrollGameCharacter>(PlayerChar);
		if (TrollChar && TrollChar->HasActiveCheckpoint())
		{
			TrollChar->RespawnAtCheckpoint();
		}
		else if (RoomStartPoint)
		{
			FVector TargetLoc = RoomStartPoint->GetActorLocation();
			FRotator TargetRot = RoomStartPoint->GetActorRotation();
			PlayerChar->TeleportTo(TargetLoc, TargetRot, false, true);
		}
		else
		{
			FVector SafeLoc = PlayerChar->GetActorLocation() - PlayerChar->GetActorForwardVector() * 300.0f;
			PlayerChar->TeleportTo(SafeLoc, PlayerChar->GetActorRotation(), false, true);
		}

		if (PlayerChar->GetCharacterMovement())
		{
			PlayerChar->GetCharacterMovement()->Velocity = FVector::ZeroVector;
		}
		break;
	}

	case EMathTrapConsequence::LaunchBackwards:
	{
		FVector BuzzerLoc = FromBuzzer ? FromBuzzer->GetActorLocation() : PlayerChar->GetActorLocation();
		FVector LaunchDir = (PlayerChar->GetActorLocation() - BuzzerLoc).GetSafeNormal();
		LaunchDir.Z = 0.45f;
		LaunchDir.Normalize();

		PlayerChar->LaunchCharacter(LaunchDir * LaunchForce, true, true);
		break;
	}

	case EMathTrapConsequence::KillPlayer:
	{
		if (ATrollGameCharacter* TrollChar = Cast<ATrollGameCharacter>(PlayerChar))
		{
			TrollChar->KillPlayer();
		}
		break;
	}
	}
}

void AMathTrapManager::UpdateScreenText(const FString& NewText, const FColor& TextColor)
{
	if (ScreenTextComp)
	{
		ScreenTextComp->SetText(FText::FromString(NewText));
		ScreenTextComp->SetTextRenderColor(TextColor);
	}
}
