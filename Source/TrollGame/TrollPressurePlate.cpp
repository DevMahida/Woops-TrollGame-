// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrollPressurePlate.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/Engine.h"

ATrollPressurePlate::ATrollPressurePlate()
{
	PrimaryActorTick.bCanEverTick = false;

	// Root trigger collision box
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	RootComponent = TriggerBox;
	TriggerBox->SetBoxExtent(FVector(100.0f, 100.0f, 20.0f));
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
	TriggerBox->SetGenerateOverlapEvents(true);

	// Optional visual mesh component
	PlateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlateMesh"));
	PlateMesh->SetupAttachment(RootComponent);
	PlateMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ScaleMultiplier = 0.5f;
	bResetVelocityOnTeleport = true;
}

void ATrollPressurePlate::BeginPlay()
{
	Super::BeginPlay();

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
