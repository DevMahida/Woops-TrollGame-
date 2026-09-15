// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MathTrapBuzzer.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UPointLightComponent;
class UAudioComponent;
class USoundBase;
class AMathTrapManager;

/**
 * Interactive buzzer/button for the Math Trap room.
 * Numbered 1 to 10, animates when pressed, and can glow for developer prank hints.
 */
UCLASS()
class TROLLGAME_API AMathTrapBuzzer : public AActor
{
	GENERATED_BODY()

public:
	AMathTrapBuzzer();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** Root scene component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Pedestal or base mesh for the buzzer */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	/** The pressable button mesh that depresses when pressed */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ButtonMesh;

	/** 3D text displaying the buzzer number (1 - 10) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTextRenderComponent> NumberText;

	/** Overlap trigger box for stepping on or walking into the buzzer */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> TriggerBox;

	/** Point light used to highlight this buzzer during Attempt 5 (fake developer help) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> HighlightLight;

	/** Audio component for buzzer clicks and sound effects */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAudioComponent> AudioComp;

public:
	/** The number assigned to this buzzer (1 to 10) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Math Trap", meta = (ClampMin = "1", ClampMax = "20"))
	int32 BuzzerNumber = 1;

	/** Sound played when this buzzer is pressed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Math Trap|Audio")
	TObjectPtr<USoundBase> PressSound;

	/** Reference to the manager coordinating the trap */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Math Trap")
	TObjectPtr<AMathTrapManager> TrapManager;

	/** Distance the button mesh depresses when pressed (in cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Math Trap|Animation")
	float DepressDistance = 4.0f;

	/** How quickly the button animates down and back up */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Math Trap|Animation")
	float AnimationSpeed = 10.0f;

	/** Whether the buzzer can currently be interacted with */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Math Trap")
	bool bCanBePressed = true;

	/** Activates or deactivates the fake developer green highlight */
	UFUNCTION(BlueprintCallable, Category = "Math Trap")
	void SetHighlight(bool bEnable);

	/** Manually triggers button press (e.g. from interact key or overlap) */
	UFUNCTION(BlueprintCallable, Category = "Math Trap")
	void PressBuzzer(ACharacter* PlayerCharacter);

	/** Returns true if this buzzer is currently highlighted */
	UFUNCTION(BlueprintPure, Category = "Math Trap")
	bool IsHighlighted() const { return bIsHighlighted; }

protected:
	/** Trigger overlap event */
	UFUNCTION()
	void OnTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Initial relative location of the button mesh */
	FVector ButtonRestLocation;

	/** Target relative location during depression */
	FVector ButtonTargetLocation;

	/** Animation state */
	bool bIsAnimating = false;
	bool bIsReturning = false;
	bool bIsHighlighted = false;

	/** Cooldown timer to prevent spamming */
	FTimerHandle CooldownTimerHandle;

	void ResetCooldown();
};
