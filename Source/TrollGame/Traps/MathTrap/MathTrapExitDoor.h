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

	/** Single door mesh fallback (swings or slides) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	/** Left sliding door panel component (e.g. Level_1_Door_1_left) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<USceneComponent> LeftDoorComponent;

	/** Right sliding door panel component (e.g. Level_1_Door_1_right) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<USceneComponent> RightDoorComponent;

	/** Status text over the door ("LOCKED" / "UNLOCKED") */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTextRenderComponent> StatusText;

public:
	/** Sound played when the door unlocks and opens */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Math Trap|Audio")
	TObjectPtr<USoundBase> UnlockSound;

	/** Target relative yaw rotation when fully open (for single swinging door fallback) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Math Trap|Door")
	float OpenYawAngle = -90.0f;

	/** Distance the left and right door panels slide apart when opening */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Math Trap|Door")
	float SlideDistance = 120.0f;

	/** Local direction vector along which doors slide apart (default lateral Y-axis) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Math Trap|Door")
	FVector SlideDirection = FVector(0.0f, 1.0f, 0.0f);

	/** How quickly the door slides or swings open */
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

	FVector LeftClosedLoc;
	FVector LeftOpenLoc;

	FVector RightClosedLoc;
	FVector RightOpenLoc;

	bool bIsOpening = false;
};
