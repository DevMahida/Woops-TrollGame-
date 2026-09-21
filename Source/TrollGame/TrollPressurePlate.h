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

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Dynamically recalculates TriggerBox size and offset based on PlateMesh size and scale */
	UFUNCTION(BlueprintCallable, Category = "Troll Trap")
	void UpdateTriggerArea();

	/** Returns the scaled local box extent of the main center plate mesh */
	UFUNCTION(BlueprintCallable, Category = "Troll Trap")
	FVector GetMainPlateExtent() const;

	/** Returns the main plate mesh component */
	UFUNCTION(BlueprintCallable, Category = "Troll Trap")
	UStaticMeshComponent* GetPlateMesh() const { return PlateMesh; }

	/** Returns all side pressure plate mesh components */
	const TArray<TObjectPtr<UStaticMeshComponent>>& GetSidePressurePlates() const { return SidePressurePlates; }

	/** Checks if a world location is stepping on the SidePressurePlate component vs main PressurePlate */
	UFUNCTION(BlueprintCallable, Category = "Troll Trap")
	bool IsLocationOnSidePlate(const FVector& WorldLocation) const;

	/** Evaluates whether player location is stepping on the pressure plate trap, and outputs whether it's a side plate */
	UFUNCTION(BlueprintCallable, Category = "Troll Trap")
	bool IsPlayerOnTrap(const FVector& PlayerLoc, bool& out_bIsSidePlate) const;

protected:
	virtual void BeginPlay() override;

	/** Visual mesh for the main pressure plate (Root component) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PlateMesh;

	/** Visual mesh components for side pressure plates (supports multiple) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TArray<TObjectPtr<UStaticMeshComponent>> SidePressurePlates;

	/** Trigger box component detecting overlaps */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> TriggerBox;

	/** The actor / target point at the start of the hallway to teleport the player to */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Troll Trap")
	TObjectPtr<AActor> TeleportTarget;

	/** Scale factor applied to the player (e.g. 0.5f = half size) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Troll Trap", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float ScaleMultiplier = 0.5f;

	/** Whether to reset player velocity when teleported */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Troll Trap")
	bool bResetVelocityOnTeleport = true;

	/** Extra height (Z extent) for the trigger box above the plate surface to detect characters walking over it */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Troll Trap|Trigger Area")
	float TriggerHeight = 50.0f;

	/** Extra X/Y padding around the pressure plate mesh bounds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Troll Trap|Trigger Area")
	float TriggerMargin = 0.0f;

	/** Z offset of the trigger box relative to the plate mesh center */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Troll Trap|Trigger Area")
	float TriggerZOffset = 25.0f;

	/** Called when an actor overlaps the trigger box */
	UFUNCTION()
	void OnTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};

