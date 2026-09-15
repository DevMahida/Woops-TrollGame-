// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MathTrapExitDoor.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class USoundBase;

/**
 * The exit door of the Math Trap room.
 * Stays locked until the player presses the true math buzzer, then swings open smoothly.
 */
UCLASS()
class TROLLGAME_API AMathTrapExitDoor : public AActor
{
	GENERATED_BODY()

public:
	AMathTrapExitDoor();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** Root scene component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Door frame mesh fixed in the wall */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> DoorFrameMesh;

	/** Moving door mesh (swings or slides) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	/** Status text over the door ("LOCKED" / "UNLOCKED") */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTextRenderComponent> StatusText;

public:
	/** Sound played when the door unlocks and opens */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Math Trap|Audio")
	TObjectPtr<USoundBase> UnlockSound;

	/** Target relative yaw rotation when fully open (e.g. -90 degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Math Trap|Door")
	float OpenYawAngle = -90.0f;

	/** How quickly the door swings open */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Math Trap|Door")
	float OpenSpeed = 4.0f;

	/** Whether the door is currently open */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Math Trap|Door")
	bool bIsOpen = false;

	/** Unlocks and opens the exit door */
	UFUNCTION(BlueprintCallable, Category = "Math Trap")
	void UnlockAndOpen();

	/** Closes and relocks the exit door */
	UFUNCTION(BlueprintCallable, Category = "Math Trap")
	void ResetDoor();

protected:
	FRotator ClosedRotation;
	FRotator OpenRotation;
	bool bIsOpening = false;
};
