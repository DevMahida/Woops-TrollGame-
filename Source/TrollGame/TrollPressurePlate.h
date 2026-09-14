// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrollPressurePlate.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/**
 * Pressure plate actor that shrinks the player and teleports them back to a designated target (e.g. hallway start).
 */
UCLASS()
class TROLLGAME_API ATrollPressurePlate : public AActor
{
	GENERATED_BODY()

public:
	ATrollPressurePlate();

protected:
	virtual void BeginPlay() override;

	/** Trigger box component detecting overlaps */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> TriggerBox;

	/** Visual mesh for the plate (optional, can be hidden) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PlateMesh;

	/** The actor / target point at the start of the hallway to teleport the player to */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Troll Trap")
	TObjectPtr<AActor> TeleportTarget;

	/** Scale factor applied to the player (e.g. 0.5f = half size) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Troll Trap", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float ScaleMultiplier = 0.5f;

	/** Whether to reset player velocity when teleported */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Troll Trap")
	bool bResetVelocityOnTeleport = true;

	/** Called when an actor overlaps the trigger box */
	UFUNCTION()
	void OnTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
