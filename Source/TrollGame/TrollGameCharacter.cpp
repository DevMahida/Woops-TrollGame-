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
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "TrollGame.h"

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
			else if (Label.Equals(TEXT("PressurePlate"), ESearchCase::IgnoreCase))
			{
				HallwayPressurePlateActor = Actor;
			}
			else if (Label.Contains(TEXT("CheckPoint"), ESearchCase::IgnoreCase) || Label.Contains(TEXT("Checkpoint"), ESearchCase::IgnoreCase))
			{
				LevelCheckpoints.Add(Actor);
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
				ScreenTextComponent->SetWorldSize(38.0f);
				ScreenTextComponent->SetHorizSpacingAdjust(1.0f);
				ScreenTextComponent->SetVertSpacingAdjust(1.25f);
				ScreenTextComponent->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
				ScreenTextComponent->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
			}
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
		float MaxRadius = FMath::Max(CPBoxExtent.X, CPBoxExtent.Y) + 80.0f;
		float DistZ = FMath::Abs(PlayerLoc.Z - CPOrigin.Z);

		if (Dist2D <= MaxRadius && DistZ <= CPBoxExtent.Z + 120.0f)
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

	// --- 3. PRESSURE PLATE TRAP CHECK ---
	if (HallwayPressurePlateActor)
	{
		FVector PlateLoc = HallwayPressurePlateActor->GetActorLocation();
		float Dist2DSq = FVector::DistSquared2D(PlayerLoc, PlateLoc);
		float DistZ = FMath::Abs(PlayerLoc.Z - PlateLoc.Z);

		if (Dist2DSq < 150.0f * 150.0f && DistZ < 120.0f)
		{
			if (!bHasTriggeredPressurePlate)
			{
				bHasTriggeredPressurePlate = true;

				// Teleport player directly to the latest active checkpoint
				FVector TargetLoc = ActiveCheckpointTransform.GetLocation();
				FRotator TargetRot = ActiveCheckpointTransform.GetRotation().Rotator();
				TeleportTo(TargetLoc, TargetRot, false, true);

				if (bHasActiveCheckpoint)
				{
					// Player has reached a level checkpoint (e.g. CheckPoint_1): restore to normal size & full speed!
					ResetPlayerScaleAndMovement();

					if (GEngine)
					{
						GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Green,
							TEXT("Teleported to Checkpoint! Normal size & speed maintained."));
					}
				}
				else
				{
					// First hallway before checkpoint 1: Shrink player to half size and halve speed & jump!
					SetActorScale3D(BaseActorScale * 0.5f);

					if (MoveComp)
					{
						MoveComp->Velocity = FVector::ZeroVector;
						MoveComp->MaxWalkSpeed = BaseMaxWalkSpeed * 0.5f;
						MoveComp->JumpZVelocity = BaseJumpZVelocity * 0.5f;
					}

					// Update the screen text for the shrink trap
					if (ScreenTextComponent)
					{
						ScreenTextComponent->SetText(FText::FromString(RoomShrinkTrapMessage));
					}

					if (GEngine)
					{
						GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Cyan,
							TEXT("Troll Trap Activated: Shrunk to half size and teleported to Start!"));
					}
				}
			}
		}
		else
		{
			if (Dist2DSq > 250.0f * 250.0f)
			{
				bHasTriggeredPressurePlate = false;
			}
		}
	}

	// --- 4. TRAP FALL FLOOR TRIGGER & REMOVE FLOOR COMPONENTS ---
	if (!bTrapFallFloorTriggered)
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
					FString::Printf(TEXT("Trap Triggered! Disappeared %d floor actors & %d components!"), 
					TrapFallFloorActors.Num(), TrapFallFloorComponents.Num()));
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

	// Restore normal player size, speed, and jump
	ResetPlayerScaleAndMovement();

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

	// Update the screen text on respawn
	if (ScreenTextComponent)
	{
		ScreenTextComponent->SetText(FText::FromString(RoomRespawnMessage));
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
