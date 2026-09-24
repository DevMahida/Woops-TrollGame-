// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MathTrapManager.generated.h"

class AMathTrapBuzzer;
class AMathTrapExitDoor;
class UTextRenderComponent;
class UPointLightComponent;
class USoundBase;

/** Struct representing a complex math equation */
USTRUCT(BlueprintType)
struct FComplexMathEquation
{
	GENERATED_BODY()

	/** Display string of the complex math equation (e.g. "(8 / 2 * (1 + 3) - 14)") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Math")
	FString EquationText;

	/** The true evaluated numeric answer of the equation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Math")
	int32 BaseAnswer = 2;

	FComplexMathEquation() {}
	FComplexMathEquation(const FString& InText, int32 InAnswer)
		: EquationText(InText), BaseAnswer(InAnswer) {}
};

/** Struct tracking generic level actor buzzers (e.g. Buzzer pedestals placed in the level) */
USTRUCT(BlueprintType)
struct FGenericBuzzerInfo
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Number = 1;

	UPROPERTY()
	TObjectPtr<AActor> BuzzerActor;

	UPROPERTY()
	TObjectPtr<USceneComponent> ButtonComponent;

	UPROPERTY()
	TObjectPtr<UTextRenderComponent> NumberText;

	UPROPERTY()
	TObjectPtr<UPointLightComponent> HighlightLight;

	FVector InitialRelLoc = FVector::ZeroVector;
	FVector TargetRelLoc = FVector::ZeroVector;
	bool bIsAnimating = false;
	bool bIsReturning = false;
	float LastPressedTime = -10.0f;
};

/** Consequence applied to player on pressing wrong buzzer */
UENUM(BlueprintType)
enum class EMathTrapConsequence : uint8
{
	TeleportToStart      UMETA(DisplayName = "Teleport to Room Start"),
	LaunchBackwards      UMETA(DisplayName = "Launch Backwards"),
	KillPlayer           UMETA(DisplayName = "Kill and Respawn Player")
};

/**
 * Central controller for the Math Trap Room.
 * Manages 10 buzzers, 1 exit door, dynamic non-repeating complex math equations,
 * screen text updates, and the 6-phase troll progression.
 */
UCLASS()
class TROLLGAME_API AMathTrapManager : public AActor
{
	GENERATED_BODY()

public:
	AMathTrapManager();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	/** Pool of complex equations (both BODMAS and intimidating math) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Math Trap|Equations")
	TArray<FComplexMathEquation> EquationPool;

	/** The exit door that opens upon solving the trap */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Math Trap|References")
	TObjectPtr<AMathTrapExitDoor> ExitDoor;

	/** Generic level door actor if ExitDoor is not a dedicated AMathTrapExitDoor */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Math Trap|References")
	TObjectPtr<AActor> GenericDoorActor;

	/** List of all dedicated AMathTrapBuzzer actors in this room (auto-detected if empty) */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Math Trap|References")
	TArray<TObjectPtr<AMathTrapBuzzer>> RoomBuzzers;

	/** List of generic level actor buzzers (auto-discovered at runtime) */
	UPROPERTY(Transient)
	TArray<FGenericBuzzerInfo> GenericBuzzers;

	/** The Screen actor in the room to display equations and taunts (e.g. Screen2) */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Math Trap|References")
	TObjectPtr<AActor> RoomScreenActor;

	/** Location to teleport the player back to on failure */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Math Trap|References")
	TObjectPtr<AActor> RoomStartPoint;

	/** Type of punishment when pressing the wrong buzzer */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Math Trap|Consequence")
	EMathTrapConsequence ConsequenceType = EMathTrapConsequence::TeleportToStart;

	/** Launch impulse force if ConsequenceType is LaunchBackwards */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Math Trap|Consequence")
	float LaunchForce = 1200.0f;

	/** Audio played on wrong buzzer selection */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Math Trap|Audio")
	TObjectPtr<USoundBase> WrongBuzzerSound;

	/** Audio played on Attempt 5 prank (cartoon laugh / airhorn) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Math Trap|Audio")
	TObjectPtr<USoundBase> PrankLaughSound;

	/** Audio played when the exit door unlocks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Math Trap|Audio")
	TObjectPtr<USoundBase> VictorySound;

	/** Current attempt number (starts at 1) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Math Trap|State")
	int32 CurrentAttempt = 1;

	/** Number of failed attempts during the real cipher solving stage (Attempt 4+) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Math Trap|State")
	int32 CipherStageFailures = 0;

	/** Whether the math trap has been activated by reaching the checkpoint */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Math Trap|State")
	bool bTrapActivated = false;

	/** Whether the math trap has been solved and completed */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Math Trap|State")
	bool bTrapCompleted = false;

	/** Currently active equation */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Math Trap|State")
	FComplexMathEquation CurrentEquation;

	/** Activates the math trap and displays the equation on Screen2 */
	UFUNCTION(BlueprintCallable, Category = "Math Trap")
	void ActivateTrap();

	/** Core logic for handling buzzer presses */
	UFUNCTION(BlueprintCallable, Category = "Math Trap")
	void ProcessBuzzerInput(int32 PressedNum, AActor* BuzzerActor, ACharacter* PlayerCharacter);

	/** Called by an AMathTrapBuzzer when pressed by the player */
	UFUNCTION(BlueprintCallable, Category = "Math Trap")
	void OnBuzzerPressed(AMathTrapBuzzer* PressedBuzzer, ACharacter* PlayerCharacter);

	/** Resets the room and picks a new non-repeating equation */
	UFUNCTION(BlueprintCallable, Category = "Math Trap")
	void ResetRoom();

	/** Picks a new equation from the pool without repeating until exhausted */
	UFUNCTION(BlueprintCallable, Category = "Math Trap")
	void PickNewEquation();

	/** Shuffles the numbers 1 to 10 randomly across all buzzers in the room */
	UFUNCTION(BlueprintCallable, Category = "Math Trap")
	void ShuffleBuzzerNumbers();

protected:
	/** Cached pointer to text render component on RoomScreenActor */
	UPROPERTY(Transient)
	TObjectPtr<UTextRenderComponent> ScreenTextComp;

	/** Center screen prompt text for look-at buzzer interaction */
	UPROPERTY(Transient)
	TObjectPtr<UTextRenderComponent> CenterPromptTextComp;

	/** Indices of equations that haven't been shown yet */
	TArray<int32> UnusedEquationIndices;

	/** The buzzer currently selected for the fake Attempt 5 developer highlight */
	UPROPERTY(Transient)
	TObjectPtr<AMathTrapBuzzer> FakeHighlightedBuzzer;

	/** Index of generic buzzer highlighted for Attempt 5 */
	int32 FakeHighlightedGenericBuzzerIndex = INDEX_NONE;

	/** Left door component or root of left door actor */
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> LeftDoorComp;

	/** Right door component or root of right door actor */
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> RightDoorComp;

	FVector LeftDoorClosedLoc = FVector::ZeroVector;
	FVector LeftDoorOpenLoc = FVector::ZeroVector;
	FVector RightDoorClosedLoc = FVector::ZeroVector;
	FVector RightDoorOpenLoc = FVector::ZeroVector;
	bool bDoorOpeningAnimation = false;

	/** Updates the text on the room screen */
	void UpdateScreenText(const FString& NewText, const FColor& TextColor = FColor(0, 255, 235, 255));

	/** Applies the configured consequence to the player */
	void ApplyConsequence(ACharacter* PlayerChar, AActor* FromBuzzer);

	/** Pre-populates the default equations pool */
	void InitializeDefaultEquations();

	/** Sets up screen and buzzers */
	void SetupRoom();
};
