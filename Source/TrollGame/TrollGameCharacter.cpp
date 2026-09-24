// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrollGameCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/BoxComponent.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "TrollGame.h"
#include "MathTrapManager.h"
#include "TrollPressurePlate.h"

ATrollGameCharacter::ATrollGameCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	
	// Create the first person mesh that will be viewed only by this character's owner
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	// Create the Camera Component	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	// configure the character comps
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	// Configure character movement
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;

	// Populate default random troll messages for shrunk void fall
	ShrunkVoidFallMessages.Add(TEXT("DEVS LOVE PLAYING WITH LITTLE PEOPLE!\n\nDid you think being tiny helped you\ndodge the void?\n\nYou fell right into the dev's\npocket-sized trap!\n\n[Size: Pocket Edition]"));
	ShrunkVoidFallMessages.Add(TEXT("POCKET-SIZED DISAPPOINTMENT!\n\nShrunk, pushed, AND dropped into\nthe void!\n\nThe holy trinity of skill issues.\n\nDevs: 1 | Mini-Noob: 0"));
	ShrunkVoidFallMessages.Add(TEXT("QUANTUM MICRO-NOOB DETECTED!\n\nYou were so small gravity didn't\neven notice you tripping over the edge!\n\nNext time bring an ant-sized parachute!"));
	ShrunkVoidFallMessages.Add(TEXT("MINIATURE DRAIN FALL!\n\nEven at half size, your brain couldn't\nfind the floor.\n\nThanks for testing the dev's\nfavorite mini-toy trap!"));

	// Enable ticking to monitor hallway trap
	PrimaryActorTick.bCanEverTick = true;
}

void ATrollGameCharacter::BeginPlay()
{
	Super::BeginPlay();

	BaseActorScale = GetActorScale3D();

	// Search the level for all relevant actors and components
	LevelCheckpoints.Empty();
	TrapFallFloorActors.Empty();
	TrapFallFloorComponents.Empty();
	TrapFallFloorTriggerActor = nullptr;
	TrapFallFloorTriggerComponent = nullptr;

	auto IsFallFloorOrSideName = [](const FString& Str) -> bool
	{
		return Str.Contains(TEXT("FallSide"), ESearchCase::IgnoreCase) ||
		       Str.Contains(TEXT("FallFloor"), ESearchCase::IgnoreCase) ||
		       Str.Contains(TEXT("FallFoor"), ESearchCase::IgnoreCase) ||
		       Str.Contains(TEXT("TrapFallFloor"), ESearchCase::IgnoreCase) ||
		       Str.Contains(TEXT("Fall_Side"), ESearchCase::IgnoreCase) ||
		       Str.Contains(TEXT("Fall_Floor"), ESearchCase::IgnoreCase);
	};

	auto IsTriggerName = [](const FString& Str) -> bool
	{
		return Str.Contains(TEXT("TrapFallFloorTrigger"), ESearchCase::IgnoreCase) ||
		       Str.Contains(TEXT("FallFloorTrigger"), ESearchCase::IgnoreCase) ||
		       Str.Contains(TEXT("FallTrigger"), ESearchCase::IgnoreCase) ||
		       Str.Contains(TEXT("FloorTrigger"), ESearchCase::IgnoreCase) ||
		       Str.Contains(TEXT("TrapTrigger"), ESearchCase::IgnoreCase) ||
		       Str.Contains(TEXT("FallSideTrigger"), ESearchCase::IgnoreCase);
	};

	// 1. Add any manually assigned floor actors from Details panel
	for (AActor* ManualActor : ManualTrapFallFloorActors)
	{
		if (ManualActor && !TrapFallFloorActors.Contains(ManualActor))
		{
			TrapFallFloorActors.Add(ManualActor);
			TArray<UPrimitiveComponent*> ManualComps;
			ManualActor->GetComponents<UPrimitiveComponent>(ManualComps);
			for (UPrimitiveComponent* MC : ManualComps)
			{
				if (MC && !TrapFallFloorComponents.Contains(MC))
				{
					TrapFallFloorComponents.Add(MC);
				}
			}
		}
	}

	UWorld* World = GetWorld();
	if (World)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (!Actor) continue;

			FString Label = Actor->GetActorLabel();
			FString Name = Actor->GetName();
			FString ActorFolderPath = Actor->GetFolderPath().ToString();

			// Match basic level actors
			if (Label.Equals(TEXT("Start"), ESearchCase::IgnoreCase))
			{
				HallwayStartActor = Actor;
			}
			else if (Label.Equals(TEXT("Screen"), ESearchCase::IgnoreCase))
			{
				RoomScreenActor = Actor;
			}
			else if (Label.Contains(TEXT("Pressure"), ESearchCase::IgnoreCase) || 
			         Name.Contains(TEXT("Pressure"), ESearchCase::IgnoreCase) || 
			         Actor->ActorHasTag(TEXT("PressurePlate")) || 
			         Actor->ActorHasTag(TEXT("Pressure")))
			{
				HallwayPressurePlateActors.AddUnique(Actor);
			}
			// Match ComeNear target actor
			else if (Label.Contains(TEXT("ComeNear"), ESearchCase::IgnoreCase) || 
			         Name.Contains(TEXT("ComeNear"), ESearchCase::IgnoreCase) ||
			         Actor->ActorHasTag(TEXT("ComeNear")))
			{
				ComeNearActor = Actor;
			}
			// Match HallwayPush2 (check 2 first)
			else if (Label.Contains(TEXT("HallwayPush2"), ESearchCase::IgnoreCase) || 
			         Name.Contains(TEXT("HallwayPush2"), ESearchCase::IgnoreCase) ||
			         Label.Contains(TEXT("Push2"), ESearchCase::IgnoreCase) ||
			         Name.Contains(TEXT("Push2"), ESearchCase::IgnoreCase) ||
			         Actor->ActorHasTag(TEXT("HallwayPush2")))
			{
				HallwayPushActor2 = Actor;
				Push2InitialLoc = Actor->GetActorLocation();
				Actor->SetActorEnableCollision(true);
				TArray<UPrimitiveComponent*> Comps;
				Actor->GetComponents<UPrimitiveComponent>(Comps);
				for (UPrimitiveComponent* Comp : Comps)
				{
					if (Comp)
					{
						Comp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
						Comp->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
					}
				}
			}
			// Match HallwayPush / HallwayPush1 / Push1 / Push
			else if (Label.Contains(TEXT("HallwayPush"), ESearchCase::IgnoreCase) || 
			         Name.Contains(TEXT("HallwayPush"), ESearchCase::IgnoreCase) ||
			         Label.Contains(TEXT("Push1"), ESearchCase::IgnoreCase) ||
			         Name.Contains(TEXT("Push1"), ESearchCase::IgnoreCase) ||
			         Actor->ActorHasTag(TEXT("HallwayPush")))
			{
				if (!HallwayPushActor1)
				{
					HallwayPushActor1 = Actor;
					Push1InitialLoc = Actor->GetActorLocation();
				}
				else if (!HallwayPushActor2 && Actor != HallwayPushActor1)
				{
					HallwayPushActor2 = Actor;
					Push2InitialLoc = Actor->GetActorLocation();
				}

				Actor->SetActorEnableCollision(true);
				TArray<UPrimitiveComponent*> Comps;
				Actor->GetComponents<UPrimitiveComponent>(Comps);
				for (UPrimitiveComponent* Comp : Comps)
				{
					if (Comp)
					{
						Comp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
						Comp->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
					}
				}
			}
			else if (Label.Contains(TEXT("CheckPoint"), ESearchCase::IgnoreCase) || 
			         Label.Contains(TEXT("Checkpoint"), ESearchCase::IgnoreCase) || 
			         Name.Contains(TEXT("CheckPoint"), ESearchCase::IgnoreCase) || 
			         Name.Contains(TEXT("Checkpoint"), ESearchCase::IgnoreCase) || 
			         Label.Contains(TEXT("checkpoint"), ESearchCase::IgnoreCase) || 
			         Name.Contains(TEXT("checkpoint"), ESearchCase::IgnoreCase))
			{
				if (!LevelCheckpoints.Contains(Actor))
				{
					LevelCheckpoints.Add(Actor);
				}
			}

			// Trap Trigger detection on Actor
			bool bIsTrigger = IsTriggerName(Label) || 
			                  IsTriggerName(Name) || 
			                  Actor->ActorHasTag(TEXT("TrapFallFloorTrigger")) ||
			                  Actor->ActorHasTag(TEXT("FallTrigger")) ||
			                  IsTriggerName(ActorFolderPath);
			if (bIsTrigger)
			{
				TrapFallFloorTriggerActor = Actor;
			}

			// Check all primitive components on this actor
			TArray<UPrimitiveComponent*> PrimComps;
			Actor->GetComponents<UPrimitiveComponent>(PrimComps);
			for (UPrimitiveComponent* Prim : PrimComps)
			{
				if (!Prim) continue;

				FString CompName = Prim->GetName();

				// Check if component is a trigger
				if (IsTriggerName(CompName) || Prim->ComponentHasTag(TEXT("TrapFallFloorTrigger")))
				{
					TrapFallFloorTriggerComponent = Prim;
					if (!TrapFallFloorTriggerActor)
					{
						TrapFallFloorTriggerActor = Actor;
					}
				}

				// Check if component is FallSide / FallFloor / FallFoor
				if (IsFallFloorOrSideName(CompName) || 
				    Prim->ComponentHasTag(TEXT("FallSide")) || 
				    Prim->ComponentHasTag(TEXT("FallFloor")) || 
				    Prim->ComponentHasTag(TEXT("FallFoor")) || 
				    Prim->ComponentHasTag(TEXT("TrapFallFloor")))
				{
					if (!TrapFallFloorComponents.Contains(Prim))
					{
						TrapFallFloorComponents.Add(Prim);
					}
					if (!bIsTrigger && Actor != this && Actor != HallwayStartActor && Actor != RoomScreenActor)
					{
						if (!TrapFallFloorActors.Contains(Actor))
						{
							TrapFallFloorActors.Add(Actor);
						}
					}
				}
			}

			// Check if actor itself matches FallSide, FallFloor, FallFoor, TrapFallFloor
			bool bIsTrapFloorActor = IsFallFloorOrSideName(Label) || 
			                         IsFallFloorOrSideName(Name) || 
			                         Actor->ActorHasTag(TEXT("FallSide")) ||
			                         Actor->ActorHasTag(TEXT("FallFloor")) ||
			                         Actor->ActorHasTag(TEXT("FallFoor")) ||
			                         Actor->ActorHasTag(TEXT("TrapFallFloor")) ||
			                         IsFallFloorOrSideName(ActorFolderPath);

			if (bIsTrapFloorActor && !bIsTrigger && Actor != this && Actor != HallwayStartActor && Actor != RoomScreenActor)
			{
				if (!TrapFallFloorActors.Contains(Actor))
				{
					TrapFallFloorActors.Add(Actor);
				}

				for (UPrimitiveComponent* Prim : PrimComps)
				{
					if (Prim && !TrapFallFloorComponents.Contains(Prim))
					{
						TrapFallFloorComponents.Add(Prim);
					}
				}

				// Include all attached actors recursively
				TArray<AActor*> AttachedChildren;
				Actor->GetAttachedActors(AttachedChildren, true, true);
				for (AActor* Child : AttachedChildren)
				{
					if (Child && !TrapFallFloorActors.Contains(Child))
					{
						TrapFallFloorActors.Add(Child);
						TArray<UPrimitiveComponent*> ChildPrims;
						Child->GetComponents<UPrimitiveComponent>(ChildPrims);
						for (UPrimitiveComponent* CP : ChildPrims)
						{
							if (CP && !TrapFallFloorComponents.Contains(CP))
							{
								TrapFallFloorComponents.Add(CP);
							}
						}
					}
				}

				// Include all child actors
				TArray<AActor*> ChildActors;
				Actor->GetAllChildActors(ChildActors, true);
				for (AActor* Child : ChildActors)
				{
					if (Child && !TrapFallFloorActors.Contains(Child))
					{
						TrapFallFloorActors.Add(Child);
						TArray<UPrimitiveComponent*> ChildPrims;
						Child->GetComponents<UPrimitiveComponent>(ChildPrims);
						for (UPrimitiveComponent* CP : ChildPrims)
						{
							if (CP && !TrapFallFloorComponents.Contains(CP))
							{
								TrapFallFloorComponents.Add(CP);
							}
						}
					}
				}
			}
		}

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 8.0f, TrapFallFloorTriggerActor ? FColor::Green : FColor::Yellow,
				FString::Printf(TEXT("Trap Trigger: %s"), TrapFallFloorTriggerActor ? *TrapFallFloorTriggerActor->GetActorLabel() : TEXT("Stepping on floor directly")));
			GEngine->AddOnScreenDebugMessage(-1, 8.0f, (TrapFallFloorActors.Num() > 0 || TrapFallFloorComponents.Num() > 0) ? FColor::Green : FColor::Red,
				FString::Printf(TEXT("Registered %d FallFloor/FallSide actors and %d components!"), TrapFallFloorActors.Num(), TrapFallFloorComponents.Num()));
		}

		// Initialize 3D Text Display on the Screen actor in the room
		if (RoomScreenActor)
		{
			ScreenTextComponent = NewObject<UTextRenderComponent>(RoomScreenActor, TEXT("ScreenWelcomeText"));
			if (ScreenTextComponent)
			{
				ScreenTextComponent->RegisterComponent();
				ScreenTextComponent->AttachToComponent(RoomScreenActor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
				
				// Prevent inheriting the screen's non-uniform scaling (Scale.X != Scale.Z)
				ScreenTextComponent->SetAbsolute(false, false, true);
				ScreenTextComponent->SetWorldScale3D(FVector(1.0f, 1.0f, 1.0f));

				// Position text slightly in front of the screen surface on the Y-direction side
				ScreenTextComponent->SetRelativeLocation(FVector(0.0f, 55.0f, 0.0f));
				ScreenTextComponent->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
				
				ScreenTextComponent->SetText(FText::FromString(RoomWelcomeMessage));
				ScreenTextComponent->SetTextRenderColor(FColor(0, 255, 235, 255)); // Bright neon cyan glow
				ScreenTextComponent->SetWorldSize(32.0f);
				ScreenTextComponent->SetHorizSpacingAdjust(1.0f);
				ScreenTextComponent->SetVertSpacingAdjust(1.20f);
				ScreenTextComponent->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
				ScreenTextComponent->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
			}
		}

		// Ensure MathTrapManager is spawned if Math Trap actors exist (Screen2 or Buzzer)
		bool bHasMathTrapActors = false;
		bool bManagerAlreadyExists = false;

		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (!Actor) continue;

			if (Actor->IsA<AMathTrapManager>())
			{
				bManagerAlreadyExists = true;
				break;
			}

			FString Label = Actor->GetActorLabel();
			if (Label.Contains(TEXT("Screen2"), ESearchCase::IgnoreCase) || 
			    Label.Contains(TEXT("Buzzer"), ESearchCase::IgnoreCase) ||
			    Label.Contains(TEXT("ButtonBuzzer"), ESearchCase::IgnoreCase))
			{
				bHasMathTrapActors = true;
			}
		}

		if (bHasMathTrapActors && !bManagerAlreadyExists)
		{
			World->SpawnActor<AMathTrapManager>(AMathTrapManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
		}
	}

	// Initialize default checkpoint to Start location if available, otherwise player's initial transform
	if (HallwayStartActor)
	{
		FVector StartOrigin, StartExtent;
		HallwayStartActor->GetActorBounds(false, StartOrigin, StartExtent);

		float CapsuleHalfHeight = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 96.0f;

		FTransform StartTransform = HallwayStartActor->GetActorTransform();
		FVector SpawnLoc = StartOrigin;
		// Position the capsule center cleanly above the top surface of the Start actor
		SpawnLoc.Z = StartOrigin.Z + StartExtent.Z + CapsuleHalfHeight + 10.0f;
		StartTransform.SetLocation(SpawnLoc);
		ActiveCheckpointTransform = StartTransform;
		ActiveCheckpointActor = HallwayStartActor;
		bHasActiveCheckpoint = false;
	}
	else
	{
		ActiveCheckpointTransform = GetActorTransform();
		ActiveCheckpointActor = nullptr;
		bHasActiveCheckpoint = false;
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		BaseJumpZVelocity = MoveComp->JumpZVelocity;
		BaseMaxWalkSpeed = MoveComp->MaxWalkSpeed;
	}
}

void ATrollGameCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// --- 1. DEAD STATUS CHECK & VOID FALL CHECK ---
	if (bIsDead)
	{
		RespawnAtCheckpoint();
		return;
	}

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	FVector PlayerLoc = GetActorLocation();

	// Check if player fell into the void
	if (PlayerLoc.Z < VoidKillZ)
	{
		bool bIsShrunk = !GetActorScale3D().Equals(BaseActorScale, 0.05f);
		if (bIsShrunk)
		{
			ShrunkVoidFallCount++;

			if (ScreenTextComponent)
			{
				if (ShrunkVoidFallCount == 1)
				{
					ScreenTextComponent->SetText(FText::FromString(ShrunkVoidFallHint1Message));
				}
				else if (ShrunkVoidFallCount == 2)
				{
					ScreenTextComponent->SetText(FText::FromString(ShrunkVoidFallHint2Message));
				}
				else // 3rd fall or beyond
				{
					ScreenTextComponent->SetText(FText::FromString(ShrunkVoidFallHint3Message));
					bHallwayPushTrapDisabled = true;
				}
			}
			else if (ShrunkVoidFallCount >= 3)
			{
				bHallwayPushTrapDisabled = true;
			}
		}

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(51, 3.0f, FColor::Red,
				TEXT("Fell into the void! Teleporting to checkpoint..."));
		}
		RespawnAtCheckpoint();
		return;
	}

	// --- 2. CHECKPOINT OVERLAP CHECK ---
	// When player steps on any CheckPoint (e.g. CheckPoint_1):
	// - Updates the active saved checkpoint to this location
	// - Restores normal player size (1.0x), speed, and jump!
	for (AActor* CheckpointActor : LevelCheckpoints)
	{
		if (!CheckpointActor) continue;

		FVector CPOrigin, CPBoxExtent;
		CheckpointActor->GetActorBounds(false, CPOrigin, CPBoxExtent);

		float Dist2D = FVector::Dist2D(PlayerLoc, CPOrigin);
		float MaxRadius = FMath::Max(150.0f, FMath::Max(CPBoxExtent.X, CPBoxExtent.Y) + 80.0f);
		float DistZ = FMath::Abs(PlayerLoc.Z - CPOrigin.Z);

		if (Dist2D <= MaxRadius && DistZ <= FMath::Max(150.0f, CPBoxExtent.Z + 120.0f))
		{
			// Check if this is a newly entered checkpoint
			bool bIsNewCP = (ActiveCheckpointActor != CheckpointActor);

			// Check if player is currently shrunk or slowed
			bool bIsShrunkOrSlowed = !GetActorScale3D().Equals(BaseActorScale, 0.05f) ||
			                         (MoveComp && (MoveComp->MaxWalkSpeed < BaseMaxWalkSpeed || MoveComp->JumpZVelocity < BaseJumpZVelocity));

			if (bIsNewCP || bIsShrunkOrSlowed)
			{
				if (bIsNewCP)
				{
					ActiveCheckpointActor = CheckpointActor;

					FTransform NewCPTransform = CheckpointActor->GetActorTransform();
					float CapsuleHalfHeight = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 96.0f;
					FVector SpawnLoc = CPOrigin;
					SpawnLoc.Z = CPOrigin.Z + CPBoxExtent.Z + CapsuleHalfHeight + 10.0f;
					NewCPTransform.SetLocation(SpawnLoc);

					SetActiveCheckpoint(NewCPTransform);

					// Activate MathTrapManager when checkpoint is reached
					UWorld* World = GetWorld();
					if (World)
					{
						for (TActorIterator<AMathTrapManager> MathIt(World); MathIt; ++MathIt)
						{
							AMathTrapManager* MathMgr = *MathIt;
							if (MathMgr && !MathMgr->bTrapActivated)
							{
								MathMgr->ActivateTrap();
							}
						}
					}

					if (GEngine)
					{
						GEngine->AddOnScreenDebugMessage(50, 4.0f, FColor::Green,
							FString::Printf(TEXT("Checkpoint Reached [%s]! Size & speed restored to normal!"), *CheckpointActor->GetActorLabel()));
					}
				}

				// Always restore normal size, speed, and jump
				ResetPlayerScaleAndMovement();
			}
			break;
		}
	}

	// --- 3. DYNAMIC PRESSURE PLATE TRAP CHECK ---
	if (HallwayPressurePlateActors.Num() == 0)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				AActor* Actor = *It;
				if (!IsValid(Actor) || Actor == this) continue;

				FString Label = Actor->GetActorLabel();
				FString Name = Actor->GetName();

				if (Actor->IsA<ATrollPressurePlate>() ||
				    Label.Contains(TEXT("Pressure"), ESearchCase::IgnoreCase) || 
				    Name.Contains(TEXT("Pressure"), ESearchCase::IgnoreCase) || 
				    Actor->ActorHasTag(TEXT("PressurePlate")))
				{
					HallwayPressurePlateActors.AddUnique(Actor);
				}
			}

			if (HallwayPressurePlateActors.Num() > 0)
			{
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Yellow,
						FString::Printf(TEXT("Found %d PressurePlate Actor(s) in Level!"), HallwayPressurePlateActors.Num()));
				}
			}
		}
	}

	if (HallwayPressurePlateActors.Num() > 0)
	{
		bool bIsOnPlate = false;
		bool bIsOnSidePlate = false;
		FString TriggeredCompName = TEXT("");

		for (AActor* PPActor : HallwayPressurePlateActors)
		{
			if (!IsValid(PPActor)) continue;

			FVector CPOrigin, CPExtent;
			PPActor->GetActorBounds(false, CPOrigin, CPExtent);

			FVector LocalOffset = PPActor->GetActorRotation().UnrotateVector(PlayerLoc - CPOrigin);
			if (LocalOffset.Z < -500.0f || LocalOffset.Z > 1000.0f)
			{
				continue;
			}

			// Find all primitive/mesh components on this actor
			TArray<UPrimitiveComponent*> Comps;
			PPActor->GetComponents<UPrimitiveComponent>(Comps);

			float MinDistSq = FLT_MAX;
			UPrimitiveComponent* ClosestComp = nullptr;

			for (UPrimitiveComponent* Comp : Comps)
			{
				if (!Comp || Comp->IsA<UBoxComponent>()) continue;

				FVector CompLoc = Comp->GetComponentLocation();
				float DistSq = FVector::DistSquared2D(PlayerLoc, CompLoc);
				if (DistSq < MinDistSq)
				{
					MinDistSq = DistSq;
					ClosestComp = Comp;
				}
			}

			float MaxRadiusSq = FMath::Square(FMath::Max(CPExtent.X, FMath::Max(CPExtent.Y, 60.0f)) + 70.0f);

			if (MinDistSq <= MaxRadiusSq || FVector::DistSquared2D(PlayerLoc, CPOrigin) <= MaxRadiusSq)
			{
				bIsOnPlate = true;

				FString CompName = ClosestComp ? ClosestComp->GetName() : PPActor->GetActorLabel();
				FString TargetActorLabel = PPActor->GetActorLabel();

				// Check if component name OR actor label contains "Side"
				if (CompName.Contains(TEXT("Side"), ESearchCase::IgnoreCase) || 
				    TargetActorLabel.Contains(TEXT("Side"), ESearchCase::IgnoreCase))
				{
					bIsOnSidePlate = true;
				}
				else
				{
					bIsOnSidePlate = false;
				}

				// If side pressure plates are disabled after 6 tries, stepping on a side plate is safe!
				if (bIsOnSidePlate && bSidePressurePlatesDisabled)
				{
					bIsOnPlate = false;
					break;
				}

				TriggeredCompName = ClosestComp ? CompName : TargetActorLabel;
				break;
			}
		}

		// Debug: Show live detection values every frame on HUD
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(100, 0.0f, FColor::White,
				FString::Printf(TEXT("PP Debug: OnPlate=%s SidePlate=%s Obj=[%s] Triggered=%s (Total Actors: %d, Tries: %d/6)"),
				bIsOnPlate ? TEXT("YES") : TEXT("NO"),
				bIsOnSidePlate ? TEXT("YES") : TEXT("NO"),
				*TriggeredCompName,
				bHasTriggeredPressurePlate ? TEXT("YES") : TEXT("NO"),
				HallwayPressurePlateActors.Num(),
				PressurePlateTriggerCount));
		}

		if (bIsOnPlate)
		{
			if (!bHasTriggeredPressurePlate)
			{
				bHasTriggeredPressurePlate = true;

				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red,
						FString::Printf(TEXT("PRESSURE PLATE TRIGGERED! [%s] Target CP=[%s]"), 
						bIsOnSidePlate ? TEXT("SIDE PANEL") : TEXT("MAIN PLATE"),
						ActiveCheckpointActor ? *ActiveCheckpointActor->GetActorLabel() : TEXT("Start")));
				}

				// 1. Teleport player directly to the latest active checkpoint
				FVector TargetLoc = ActiveCheckpointTransform.GetLocation();
				FRotator TargetRot = ActiveCheckpointTransform.GetRotation().Rotator();
				TeleportTo(TargetLoc, TargetRot, false, true);

				PressurePlateTriggerCount++;
				bool bWasAlreadyShrunk = !GetActorScale3D().Equals(BaseActorScale, 0.05f);
				int32 CurrentPlateType = bIsOnSidePlate ? 2 : 1;
				bool bIsSamePlateRepeated = (bWasAlreadyShrunk && LastShrinkSourcePlate == CurrentPlateType);

				if (bIsSamePlateRepeated)
				{
					ConsecutiveSamePlateCount++;
				}
				else
				{
					ConsecutiveSamePlateCount = 0;
				}

				// Determine message based on attempts, repeated presses, and plate type
				FString DisplayMsg;
				if (PressurePlateTriggerCount >= 6)
				{
					bSidePressurePlatesDisabled = true;
					DisplayMsg = RoomSidePlatesDisabled6thMessage;
				}
				else if (bIsSamePlateRepeated && ConsecutiveSamePlateCount >= 2)
				{
					// Repeated pressing of the same plate: special roast with speed punishment
					DisplayMsg = RoomRepeatPlatePunishMessage;
				}
				else if (!bWasAlreadyShrunk)
				{
					if (bIsOnSidePlate)
					{
						DisplayMsg = RoomSidePlateShrinkMessage;
					}
					else
					{
						DisplayMsg = RoomShrinkTrapMessage;
					}
				}
				else
				{
					// Already shrunk: Context-aware roast
					if (LastShrinkSourcePlate == 1 && bIsOnSidePlate)
					{
						// Shrunk from main plate, now tried side plate to avoid it
						DisplayMsg = RoomShrunkMainThenSideMessage;
					}
					else if (LastShrinkSourcePlate == 2 && bIsOnSidePlate)
					{
						// Shrunk from side plate, stepped on side plate AGAIN
						DisplayMsg = RoomShrunkSideRepeatMessage;
					}
					else if (LastShrinkSourcePlate == 2 && !bIsOnSidePlate)
					{
						// Shrunk from side plate, now stepped on main plate
						DisplayMsg = RoomShrunkSideThenMainMessage;
					}
					else // LastShrinkSourcePlate == 1 && !bIsOnSidePlate (or fallback)
					{
						// Shrunk from main plate, stepped on main plate AGAIN
						DisplayMsg = RoomShrunkMainRepeatMessage;
					}
				}

				LastShrinkSourcePlate = CurrentPlateType;

				// 2. Always shrink player to half size and reduce speed & jump
				SetActorScale3D(BaseActorScale * 0.5f);

				if (MoveComp)
				{
					MoveComp->Velocity = FVector::ZeroVector;

					// Progressive speed reduction punishment on repeated same-plate presses
					float SpeedMultiplier = 0.5f;
					if (bIsSamePlateRepeated && ConsecutiveSamePlateCount > 0)
					{
						SpeedMultiplier = FMath::Max(0.15f, 0.5f - (ConsecutiveSamePlateCount * 0.1f));
					}

					MoveComp->MaxWalkSpeed = BaseMaxWalkSpeed * SpeedMultiplier;
					MoveComp->JumpZVelocity = BaseJumpZVelocity * SpeedMultiplier;
				}

				// 3. Update the screen text for the shrink trap
				if (ScreenTextComponent)
				{
					ScreenTextComponent->SetText(FText::FromString(DisplayMsg));
				}

				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Cyan,
						FString::Printf(TEXT("Troll Trap Activated (%s) | Attempt %d/6: Shrunk & teleported to Checkpoint!"),
						bIsOnSidePlate ? TEXT("Side Panel") : TEXT("Main Plate"),
						PressurePlateTriggerCount));
				}
			}
		}
		else
		{
			bHasTriggeredPressurePlate = false;
		}
	}

	// --- 4. TRAP FALL FLOOR TRIGGER & HALLWAY PUSHERS ---
	if (!bHallwayPushTrapDisabled && !bTrapFallFloorTriggered && !bHallwayPushDelayActive && !bHallwayPushMoving)
	{
		bool bTriggerActivated = false;

		float CapsuleRadius = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleRadius() : 40.0f;
		float CapsuleHalfHeight = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 96.0f;
		float PlayerFeetZ = PlayerLoc.Z - CapsuleHalfHeight;

		// 1. Check if player overlaps TrapFallFloorTriggerActor
		if (TrapFallFloorTriggerActor)
		{
			// Native engine overlap check
			if (IsOverlappingActor(TrapFallFloorTriggerActor))
			{
				bTriggerActivated = true;
			}
			else
			{
				FVector TrigOrigin, TrigExtent;
				TrapFallFloorTriggerActor->GetActorBounds(false, TrigOrigin, TrigExtent);

				// Precise footprint check respecting the exact scaled size + player capsule radius
				bool bInside2D = (FMath::Abs(PlayerLoc.X - TrigOrigin.X) <= (TrigExtent.X + CapsuleRadius)) &&
				                 (FMath::Abs(PlayerLoc.Y - TrigOrigin.Y) <= (TrigExtent.Y + CapsuleRadius));

				// Vertical range check (supports stepping on top of buried triggers as well as walking through them)
				bool bInsideZ = (PlayerFeetZ <= (TrigOrigin.Z + TrigExtent.Z + 120.0f)) && 
				                ((PlayerLoc.Z + CapsuleHalfHeight) >= (TrigOrigin.Z - TrigExtent.Z - 50.0f));

				if (bInside2D && bInsideZ)
				{
					bTriggerActivated = true;
				}
			}
		}
		// 2. Check if player overlaps TrapFallFloorTriggerComponent
		else if (TrapFallFloorTriggerComponent)
		{
			if (GetCapsuleComponent() && GetCapsuleComponent()->IsOverlappingComponent(TrapFallFloorTriggerComponent))
			{
				bTriggerActivated = true;
			}
			else
			{
				FBoxSphereBounds Bounds = TrapFallFloorTriggerComponent->Bounds;
				FVector TrigOrigin = Bounds.Origin;
				FVector TrigExtent = Bounds.BoxExtent;

				bool bInside2D = (FMath::Abs(PlayerLoc.X - TrigOrigin.X) <= (TrigExtent.X + CapsuleRadius)) &&
				                 (FMath::Abs(PlayerLoc.Y - TrigOrigin.Y) <= (TrigExtent.Y + CapsuleRadius));
				bool bInsideZ = (PlayerFeetZ <= (TrigOrigin.Z + TrigExtent.Z + 120.0f)) && 
				                ((PlayerLoc.Z + CapsuleHalfHeight) >= (TrigOrigin.Z - TrigExtent.Z - 50.0f));

				if (bInside2D && bInsideZ)
				{
					bTriggerActivated = true;
				}
			}
		}
		// 3. Fallback: Only if NO TrapFallFloorTrigger was found in the level, step on floor directly
		else
		{
			for (AActor* FloorActor : TrapFallFloorActors)
			{
				if (!FloorActor) continue;
				FVector FloorOrigin, FloorExtent;
				FloorActor->GetActorBounds(false, FloorOrigin, FloorExtent);

				bool bWithin2D = (FMath::Abs(PlayerLoc.X - FloorOrigin.X) <= (FloorExtent.X + 20.0f)) &&
				                 (FMath::Abs(PlayerLoc.Y - FloorOrigin.Y) <= (FloorExtent.Y + 20.0f));
				bool bWithinZ = (PlayerLoc.Z >= (FloorOrigin.Z + FloorExtent.Z - 50.0f)) &&
				                (PlayerLoc.Z <= (FloorOrigin.Z + FloorExtent.Z + 200.0f));

				if (bWithin2D && bWithinZ)
				{
					bTriggerActivated = true;
					break;
				}
			}
		}

		if (bTriggerActivated)
		{
			bTrapFallFloorTriggered = true;
			bHallwayPushMoving = true;
			bHallwayPushDelayActive = false;

			// Instantly remove and disable all TrapFallFloor / FallFloor / FallSide actors
			for (AActor* FloorActor : TrapFallFloorActors)
			{
				if (!FloorActor) continue;

				FloorActor->SetActorHiddenInGame(true);
				FloorActor->SetActorEnableCollision(false);

				TArray<UPrimitiveComponent*> PrimComps;
				FloorActor->GetComponents<UPrimitiveComponent>(PrimComps);
				for (UPrimitiveComponent* Prim : PrimComps)
				{
					if (Prim)
					{
						Prim->SetVisibility(false, true);
						Prim->SetHiddenInGame(true, true);
						Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
						Prim->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
					}
				}
			}

			// Also disable any independently registered primitive components
			for (UPrimitiveComponent* Prim : TrapFallFloorComponents)
			{
				if (Prim)
				{
					Prim->SetVisibility(false, true);
					Prim->SetHiddenInGame(true, true);
					Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					Prim->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
				}
			}

			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Orange,
					FString::Printf(TEXT("Trap Triggered! Disappeared %d floor actors & Pushers moving instantly to ComeNear!"), 
					TrapFallFloorActors.Num()));
			}
		}
	}

	// Process movement of HallwayPush and HallwayPush2 actors towards ComeNear
	if (bHallwayPushMoving)
	{
		FVector TargetCenter = FVector::ZeroVector;
		bool bHasTarget = false;

		if (ComeNearActor)
		{
			TargetCenter = ComeNearActor->GetActorLocation();
			bHasTarget = true;
		}
		else if (HallwayPushActor1 && HallwayPushActor2)
		{
			TargetCenter = (Push1InitialLoc + Push2InitialLoc) * 0.5f;
			bHasTarget = true;
		}
		else if (HallwayPushActor1)
		{
			TargetCenter = Push1InitialLoc + FVector(0, 600, 0);
			bHasTarget = true;
		}
		else if (HallwayPushActor2)
		{
			TargetCenter = Push2InitialLoc - FVector(0, 600, 0);
			bHasTarget = true;
		}

		if (bHasTarget)
		{
			if (HallwayPushActor1)
			{
				FVector Loc1 = HallwayPushActor1->GetActorLocation();
				FVector Dir1 = (TargetCenter - Push1InitialLoc).GetSafeNormal2D();
				if (Dir1.IsNearlyZero())
				{
					Dir1 = FVector(0.0f, 1.0f, 0.0f);
				}
				FVector Target1 = TargetCenter + (Dir1 * 350.0f);
				Target1.Z = Push1InitialLoc.Z;

				FVector NewLoc1 = FMath::VInterpConstantTo(Loc1, Target1, DeltaSeconds, HallwayPushSpeed);
				FVector Delta1 = NewLoc1 - Loc1;

				// Set location without bSweep so static walls/floors won't block pusher movement
				HallwayPushActor1->SetActorLocation(NewLoc1, false, nullptr, ETeleportType::TeleportPhysics);

				// Accurate pusher bounding box check + extra margin
				FVector POrigin1, PExtent1;
				HallwayPushActor1->GetActorBounds(false, POrigin1, PExtent1);
				bool bInZone1 = (FMath::Abs(PlayerLoc.X - POrigin1.X) <= (PExtent1.X + 80.0f)) &&
				                (FMath::Abs(PlayerLoc.Y - POrigin1.Y) <= (PExtent1.Y + 80.0f)) &&
				                (FMath::Abs(PlayerLoc.Z - POrigin1.Z) <= (PExtent1.Z + 120.0f));

				// Solid displacement: if player overlaps or is close to pusher, push player along
				if (IsOverlappingActor(HallwayPushActor1) || bInZone1 || FVector::DistSquared2D(PlayerLoc, NewLoc1) < 62500.0f)
				{
					AddActorWorldOffset(Delta1 * 1.25f, true);
				}
			}

			if (HallwayPushActor2)
			{
				FVector Loc2 = HallwayPushActor2->GetActorLocation();
				FVector Dir2 = (TargetCenter - Push2InitialLoc).GetSafeNormal2D();
				if (Dir2.IsNearlyZero())
				{
					Dir2 = FVector(0.0f, -1.0f, 0.0f);
				}
				FVector Target2 = TargetCenter + (Dir2 * 350.0f);
				Target2.Z = Push2InitialLoc.Z;

				FVector NewLoc2 = FMath::VInterpConstantTo(Loc2, Target2, DeltaSeconds, HallwayPushSpeed);
				FVector Delta2 = NewLoc2 - Loc2;

				// Set location without bSweep so static walls/floors won't block pusher movement
				HallwayPushActor2->SetActorLocation(NewLoc2, false, nullptr, ETeleportType::TeleportPhysics);

				// Accurate pusher bounding box check + extra margin
				FVector POrigin2, PExtent2;
				HallwayPushActor2->GetActorBounds(false, POrigin2, PExtent2);
				bool bInZone2 = (FMath::Abs(PlayerLoc.X - POrigin2.X) <= (PExtent2.X + 80.0f)) &&
				                (FMath::Abs(PlayerLoc.Y - POrigin2.Y) <= (PExtent2.Y + 80.0f)) &&
				                (FMath::Abs(PlayerLoc.Z - POrigin2.Z) <= (PExtent2.Z + 120.0f));

				// Solid displacement: if player overlaps or is close to pusher, push player along
				if (IsOverlappingActor(HallwayPushActor2) || bInZone2 || FVector::DistSquared2D(PlayerLoc, NewLoc2) < 62500.0f)
				{
					AddActorWorldOffset(Delta2 * 1.25f, true);
				}
			}

			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(99, 0.0f, FColor::Cyan,
					FString::Printf(TEXT("PUSHER LIVE MOVING: Target=[%s] P1=[%s] P2=[%s]"),
					ComeNearActor ? *ComeNearActor->GetActorLabel() : TEXT("Midpoint"),
					HallwayPushActor1 ? *HallwayPushActor1->GetActorLabel() : TEXT("NONE"),
					HallwayPushActor2 ? *HallwayPushActor2->GetActorLabel() : TEXT("NONE")));
			}
		}
	}
}

void ATrollGameCharacter::KillPlayer()
{
	bIsDead = true;
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Red, TEXT("Player Died! Teleporting to CheckPoint..."));
	}
	RespawnAtCheckpoint();
}

void ATrollGameCharacter::RespawnAtCheckpoint()
{
	bIsDead = false;

	// Teleport to latest saved checkpoint transform
	FVector DestLoc = ActiveCheckpointTransform.GetLocation();
	FRotator DestRot = ActiveCheckpointTransform.GetRotation().Rotator();
	TeleportTo(DestLoc, DestRot, false, true);

	// Zero out movement velocity on respawning so player spawns cleanly
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->Velocity = FVector::ZeroVector;
	}

	// NOTE: Player scale & movement stats are preserved on void fall!
	// If the player was shrunk when falling into the void, they remain shrunk.
	// If the player was normal sized, they remain normal sized.

	// Reset HallwayPush trap state & actor locations
	bHallwayPushDelayActive = false;
	bHallwayPushMoving = false;
	HallwayPushDelayTimer = 0.0f;

	if (HallwayPushActor1)
	{
		HallwayPushActor1->SetActorLocation(Push1InitialLoc, false, nullptr, ETeleportType::TeleportPhysics);
	}
	if (HallwayPushActor2)
	{
		HallwayPushActor2->SetActorLocation(Push2InitialLoc, false, nullptr, ETeleportType::TeleportPhysics);
	}

	// Restore and re-enable all TrapFallFloor actors and components
	bTrapFallFloorTriggered = false;
	for (AActor* FloorActor : TrapFallFloorActors)
	{
		if (!FloorActor) continue;

		FloorActor->SetActorHiddenInGame(false);
		FloorActor->SetActorEnableCollision(true);

		TArray<UPrimitiveComponent*> PrimComps;
		FloorActor->GetComponents<UPrimitiveComponent>(PrimComps);
		for (UPrimitiveComponent* Prim : PrimComps)
		{
			if (Prim)
			{
				Prim->SetVisibility(true, true);
				Prim->SetHiddenInGame(false, true);
				Prim->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				Prim->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
			}
		}
	}

	for (UPrimitiveComponent* Prim : TrapFallFloorComponents)
	{
		if (Prim)
		{
			Prim->SetVisibility(true, true);
			Prim->SetHiddenInGame(false, true);
			Prim->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Prim->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		}
	}

	// Update the screen text on respawn (only switch to RespawnMessage if player is normal sized)
	if (ScreenTextComponent)
	{
		bool bIsShrunk = !GetActorScale3D().Equals(BaseActorScale, 0.05f);
		if (!bIsShrunk)
		{
			ScreenTextComponent->SetText(FText::FromString(RoomRespawnMessage));
		}
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Yellow, TEXT("Respawned at Checkpoint!"));
	}
}

void ATrollGameCharacter::ResetPlayerScaleAndMovement()
{
	// Restore full size
	SetActorScale3D(BaseActorScale);

	// Restore movement speed and jump
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->Velocity = FVector::ZeroVector;
		MoveComp->MaxWalkSpeed = BaseMaxWalkSpeed;
		MoveComp->JumpZVelocity = BaseJumpZVelocity;
	}

	ShrunkVoidFallCount = 0;
	LastShrinkSourcePlate = 0;
	ConsecutiveSamePlateCount = 0;
}

void ATrollGameCharacter::SetActiveCheckpoint(const FTransform& NewCheckpoint)
{
	ActiveCheckpointTransform = NewCheckpoint;
	bHasActiveCheckpoint = true;
}

void ATrollGameCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ATrollGameCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ATrollGameCharacter::DoJumpEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATrollGameCharacter::MoveInput);

		// Looking/Aiming
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATrollGameCharacter::LookInput);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ATrollGameCharacter::LookInput);
	}
	else
	{
		UE_LOG(LogTrollGame, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void ATrollGameCharacter::MoveInput(const FInputActionValue& Value)
{
	// get the Vector2D move axis
	FVector2D MovementVector = Value.Get<FVector2D>();

	// pass the axis values to the move input
	DoMove(MovementVector.X, MovementVector.Y);

}

void ATrollGameCharacter::LookInput(const FInputActionValue& Value)
{
	// get the Vector2D look axis
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// pass the axis values to the aim input
	DoAim(LookAxisVector.X, LookAxisVector.Y);

}

void ATrollGameCharacter::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		// pass the rotation inputs
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ATrollGameCharacter::DoMove(float Right, float Forward)
{
	if (GetController())
	{
		// pass the move inputs
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

void ATrollGameCharacter::DoJumpStart()
{
	// pass Jump to the character
	Jump();
}

void ATrollGameCharacter::DoJumpEnd()
{
	// pass StopJumping to the character
	StopJumping();
}
