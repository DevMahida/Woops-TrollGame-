// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MathTrapManager.generated.h"

class AMathTrapBuzzer;
class AMathTrapExitDoor;
class UTextRenderComponent;
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

public:
	/** Pool of complex equations (both BODMAS and intimidating math) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Math Trap|Equations")
	TArray<FComplexMathEquation> EquationPool;

	/** The exit door that opens upon solving the trap */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Math Trap|References")
	TObjectPtr<AMathTrapExitDoor> ExitDoor;

	/** List of all buzzers in this room (auto-detected if empty) */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Math Trap|References")
	TArray<TObjectPtr<AMathTrapBuzzer>> RoomBuzzers;

	/** The Screen actor in the room to display equations and taunts */
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

	/** Currently active equation */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Math Trap|State")
	FComplexMathEquation CurrentEquation;

	/** Called by an AMathTrapBuzzer when pressed by the player */
	UFUNCTION(BlueprintCallable, Category = "Math Trap")
	void OnBuzzerPressed(AMathTrapBuzzer* PressedBuzzer, ACharacter* PlayerCharacter);

	/** Resets the room and picks a new non-repeating equation */
	UFUNCTION(BlueprintCallable, Category = "Math Trap")
	void ResetRoom();

	/** Picks a new equation from the pool without repeating until exhausted */
	UFUNCTION(BlueprintCallable, Category = "Math Trap")
	void PickNewEquation();

protected:
	/** Cached pointer to text render component on RoomScreenActor */
	UPROPERTY(Transient)
	TObjectPtr<UTextRenderComponent> ScreenTextComp;

	/** Indices of equations that haven't been shown yet */
	TArray<int32> UnusedEquationIndices;

	/** The buzzer currently selected for the fake Attempt 5 developer highlight */
	UPROPERTY(Transient)
	TObjectPtr<AMathTrapBuzzer> FakeHighlightedBuzzer;

	/** Updates the text on the room screen */
	void UpdateScreenText(const FString& NewText, const FColor& TextColor = FColor(0, 255, 235, 255));

	/** Applies the configured consequence to the player */
	void ApplyConsequence(ACharacter* PlayerChar, AMathTrapBuzzer* FromBuzzer);

	/** Pre-populates the default equations pool */
	void InitializeDefaultEquations();

	/** Sets up screen and buzzers */
	void SetupRoom();
};
