// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrollPressurePlate.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/Engine.h"

ATrollPressurePlate::ATrollPressurePlate()
{
	PrimaryActorTick.bCanEverTick = false;

	// Visual mesh component for main pressure plate as root component
	PlateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlateMesh"));
	RootComponent = PlateMesh;

	// Trigger collision box attached to RootComponent (PlateMesh)
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(RootComponent);
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
	TriggerBox->SetGenerateOverlapEvents(true);

	ScaleMultiplier = 0.5f;
	bResetVelocityOnTeleport = true;
	TriggerHeight = 50.0f;
	TriggerMargin = 0.0f;
	TriggerZOffset = 25.0f;
}

void ATrollPressurePlate::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateTriggerArea();
}

void ATrollPressurePlate::UpdateTriggerArea()
{
	if (!TriggerBox)
	{
		return;
	}

	// Calculate combined bounding box of all visual/mesh components in the actor (excluding TriggerBox)
	FBox CombinedBox(ForceInit);

	TArray<UPrimitiveComponent*> Primitives;
	GetComponents<UPrimitiveComponent>(Primitives);

	for (UPrimitiveComponent* Prim : Primitives)
	{
		if (Prim && Prim != TriggerBox)
		{
			// Transform component local bounds into RootComponent space
			FTransform RelativeXform = Prim->GetRelativeTransform();
			FBoxSphereBounds PrimBounds = Prim->GetLocalBounds();
			FBox ComponentBox = PrimBounds.GetBox().TransformBy(RelativeXform);
			CombinedBox += ComponentBox;
		}
	}

	FVector LocalMeshExtent = FVector(100.0f, 100.0f, 20.0f);
	FVector LocalMeshOrigin = FVector::ZeroVector;

	if (CombinedBox.IsValid)
	{
		CombinedBox.GetCenterAndExtents(LocalMeshOrigin, LocalMeshExtent);
	}
	else if (PlateMesh && PlateMesh->GetStaticMesh())
	{
		FBoxSphereBounds MeshBounds = PlateMesh->GetStaticMesh()->GetBounds();
		LocalMeshExtent = MeshBounds.BoxExtent;
		LocalMeshOrigin = MeshBounds.Origin;
	}

	// Calculate dynamic box extent for TriggerBox to match all combined plate components
	float ExtentX = FMath::Max(LocalMeshExtent.X + TriggerMargin, 5.0f);
	float ExtentY = FMath::Max(LocalMeshExtent.Y + TriggerMargin, 5.0f);
	float ExtentZ = FMath::Max(LocalMeshExtent.Z + TriggerHeight, 10.0f);

	TriggerBox->SetBoxExtent(FVector(ExtentX, ExtentY, ExtentZ));

	// Position TriggerBox relative to combined center origin + vertical offset
	FVector RelativeLoc = LocalMeshOrigin + FVector(0.0f, 0.0f, TriggerZOffset);
	TriggerBox->SetRelativeLocation(RelativeLoc);
}

FVector ATrollPressurePlate::GetMainPlateExtent() const
{
	if (PlateMesh && PlateMesh->GetStaticMesh())
	{
		return PlateMesh->GetStaticMesh()->GetBounds().BoxExtent * PlateMesh->GetRelativeScale3D();
	}
	return FVector(100.0f, 100.0f, 20.0f);
}

bool ATrollPressurePlate::IsLocationOnSidePlate(const FVector& WorldLocation) const
{
	bool bIsSide = false;
	IsPlayerOnTrap(WorldLocation, bIsSide);
	return bIsSide;
}

bool ATrollPressurePlate::IsPlayerOnTrap(const FVector& PlayerLoc, bool& out_bIsSidePlate) const
{
	out_bIsSidePlate = false;

	// Check vertical height bounds: trap triggers within Z range [-500..1000]
	FVector CPOrigin, CPExtent;
	GetActorBounds(false, CPOrigin, CPExtent);
	FVector ActorLocalOffset = GetActorRotation().UnrotateVector(PlayerLoc - CPOrigin);

	if (ActorLocalOffset.Z < -500.0f || ActorLocalOffset.Z > 1000.0f)
	{
		return false;
	}

	// 1. Calculate 2D distance to Main Plate component ("PressurePlate" / "PlateMesh")
	float DistToMainSq = FLT_MAX;
	if (PlateMesh)
	{
		FVector MainLoc = PlateMesh->GetComponentLocation();
		DistToMainSq = FVector::DistSquared2D(PlayerLoc, MainLoc);
	}

	// 2. Calculate 2D distance to closest Side Plate component ("SidePressurePlate", "SidePressurePlate1")
	float MinSideDistSq = FLT_MAX;
	for (const TObjectPtr<UStaticMeshComponent>& SidePlate : SidePressurePlates)
	{
		if (SidePlate)
		{
			FVector SideLoc = SidePlate->GetComponentLocation();
			float DistSq = FVector::DistSquared2D(PlayerLoc, SideLoc);
			if (DistSq < MinSideDistSq)
			{
				MinSideDistSq = DistSq;
			}
		}
	}

	// 3. Determine horizontal trigger threshold radius
	float TouchMargin = 65.0f;
	float MaxMainRadiusSq = FMath::Square(FMath::Max(CPExtent.X, 60.0f) + TouchMargin);
	float MaxSideRadiusSq = FMath::Square(FMath::Max(CPExtent.Y, 60.0f) + TouchMargin);

	bool bNearMain = (DistToMainSq <= MaxMainRadiusSq);
	bool bNearSide = (MinSideDistSq <= MaxSideRadiusSq);
	bool bNearActorCenter = (FVector::DistSquared2D(PlayerLoc, CPOrigin) <= MaxMainRadiusSq);

	if (!bNearMain && !bNearSide && !bNearActorCenter)
	{
		return false;
	}

	// 4. PLAYER IS ON TRAP! Compare 2D distances to accurately identify Main Plate vs Side Plate!
	if (MinSideDistSq < DistToMainSq)
	{
		out_bIsSidePlate = true;
	}
	else
	{
		out_bIsSidePlate = false;
	}

	return true;
}

void ATrollPressurePlate::BeginPlay()
{
	Super::BeginPlay();

	SidePressurePlates.Empty();

	TArray<UStaticMeshComponent*> AllMeshComps;
	GetComponents<UStaticMeshComponent>(AllMeshComps);

	for (UStaticMeshComponent* MeshComp : AllMeshComps)
	{
		if (!MeshComp) continue;

		FString CompName = MeshComp->GetName();

		// Check if this component is titled "PressurePlate" or "PlateMesh"
		if (CompName.Equals(TEXT("PressurePlate"), ESearchCase::IgnoreCase) || 
		    CompName.Equals(TEXT("PlateMesh"), ESearchCase::IgnoreCase) ||
		    CompName.Equals(TEXT("MainPlate"), ESearchCase::IgnoreCase))
		{
			PlateMesh = MeshComp;
		}
		else
		{
			SidePressurePlates.Add(MeshComp);
		}
	}

	// If PlateMesh was not found by exact name, use RootComponent or first mesh
	if (!PlateMesh && AllMeshComps.Num() > 0)
	{
		PlateMesh = AllMeshComps[0];
		SidePressurePlates.Remove(PlateMesh);
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Green,
			FString::Printf(TEXT("TrollPressurePlate: MainPlate=[%s], Found %d SidePlate(s)"), 
			PlateMesh ? *PlateMesh->GetName() : TEXT("None"), 
			SidePressurePlates.Num()));
	}

	UpdateTriggerArea();

	if (TriggerBox)
	{
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ATrollPressurePlate::OnTriggerOverlap);
	}
}

void ATrollPressurePlate::OnTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this)
	{
		return;
	}

	// Verify the overlapping actor is a Character (the player)
	ACharacter* Character = Cast<ACharacter>(OtherActor);
	if (!Character)
	{
		return;
	}

	// 1. Shrink the character
	FVector CurrentScale = Character->GetActorScale3D();
	FVector NewScale = CurrentScale * ScaleMultiplier;
	Character->SetActorScale3D(NewScale);

	// 2. Teleport the character to the target location if set
	if (TeleportTarget)
	{
		FVector DestLocation = TeleportTarget->GetActorLocation();
		FRotator DestRotation = TeleportTarget->GetActorRotation();

		// Keep character capsule bottom on the ground level
		Character->TeleportTo(DestLocation, DestRotation, false, true);

		// 3. Reset velocity so player doesn't fly out of control
		if (bResetVelocityOnTeleport && Character->GetCharacterMovement())
		{
			Character->GetCharacterMovement()->Velocity = FVector::ZeroVector;
		}
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow,
				TEXT("[TrollPressurePlate] No TeleportTarget selected in Details panel!"));
		}
	}
}

