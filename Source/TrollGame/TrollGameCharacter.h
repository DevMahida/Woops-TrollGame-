// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "TrollGameCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A basic first person character
 */
UCLASS(abstract)
class ATrollGameCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: first person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonMesh;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* MouseLookAction;
	
public:
	ATrollGameCharacter();

protected:

	/** Called from Input Actions for movement input */
	void MoveInput(const FInputActionValue& Value);

	/** Called from Input Actions for looking input */
	void LookInput(const FInputActionValue& Value);

	/** Handles aim inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoAim(float Yaw, float Pitch);

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles jump start inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump end inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

	/** Cached pointer to the Start location actor */
	UPROPERTY(Transient)
	TObjectPtr<AActor> HallwayStartActor;

	/** Cached pointers to all PressurePlate actors (main + side plates) in the level */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> HallwayPressurePlateActors;

	/** Cached pointer to the Room Screen actor */
	UPROPERTY(Transient)
	TObjectPtr<AActor> RoomScreenActor;

	/** Text render component attached directly to the Screen actor */
	UPROPERTY(Transient)
	TObjectPtr<class UTextRenderComponent> ScreenTextComponent;

	/** Message displayed on the room screen */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Screen")
	FString RoomWelcomeMessage = TEXT("WELCOME TO \"WHOOPS!\"\n\nThink you're smart enough to beat this? We doubt it.\n\nTo begin, all you have to do is walk down\nthe hallway to the next room.\n\nNothing complicated.\nEven you can't mess that up... right?");

	/** Message displayed on the room screen after player falls or respawns */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Screen")
	FString RoomRespawnMessage = TEXT("SO... YOU MESSED UP HUH?\n\nWe said \"walk down the hallway\",\nnot \"jump into the abyss\".\n\nTake a deep breath and try again.\nWe'll wait.");

	/** Message displayed on the room screen when player is shrunk by the pressure plate trap */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Screen")
	FString RoomShrinkTrapMessage = TEXT("HONEY, I SHRUNK THE NOOB!\n\nNow your body matches your skill level.\n\nGood luck making that jump\nwith those tiny legs!\n\n[Size: Micro | Skill Level: Noob]");

	/** Message displayed on the room screen when player steps on a side pressure plate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Screen")
	FString RoomSidePlateShrinkMessage = TEXT("LMAO! You really thought you cooked\nby walking on the side panel?\n\nAww, sad life for you!\n\nYour body is now officially as tiny\nas your brain and your gaming skills!\n\n[Size: Ant-Size | Skill Level: Trash]");

	/** Cached list of all checkpoint actors found in the level (e.g. CheckPoint_1) */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> LevelCheckpoints;

	/** Cached pointer to the TrapFallFloorTrigger actor */
	UPROPERTY(Transient)
	TObjectPtr<AActor> TrapFallFloorTriggerActor;

	/** Cached pointer to the TrapFallFloorTrigger component if any */
	UPROPERTY(Transient)
	TObjectPtr<class UPrimitiveComponent> TrapFallFloorTriggerComponent;

	/** Optional manual list of floor actors to disappear on trap trigger (can be assigned in Details panel) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap")
	TArray<TObjectPtr<AActor>> ManualTrapFallFloorActors;

	/** Cached list of all actors in the TrapFallFloor / FallFloor / FallSide group */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> TrapFallFloorActors;

	/** Cached list of all primitive components in the TrapFallFloor / FallFloor / FallSide group */
	UPROPERTY(Transient)
	TArray<TObjectPtr<class UPrimitiveComponent>> TrapFallFloorComponents;

	/** Whether the TrapFallFloor trap has been triggered */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trap")
	bool bTrapFallFloorTriggered = false;

	/** The transform of the most recently saved checkpoint */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Checkpoint")
	FTransform ActiveCheckpointTransform;

	/** The actor representing the currently active checkpoint */
	UPROPERTY(Transient)
	TObjectPtr<AActor> ActiveCheckpointActor;

	/** Whether the player has activated at least one checkpoint */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Checkpoint")
	bool bHasActiveCheckpoint = false;

	/** Player dead status */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Status")
	bool bIsDead = false;

	/** Default base jump velocity before any modifiers */
	float BaseJumpZVelocity = 600.0f;

	/** Default base walk speed before any modifiers */
	float BaseMaxWalkSpeed = 600.0f;

	/** Z height threshold below which the player is considered fallen into the void */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	float VoidKillZ = -400.0f;

	/** Default base scale before shrinking */
	FVector BaseActorScale = FVector::OneVector;

	/** Flag to prevent multiple rapid triggers in the same overlap */
	bool bHasTriggeredPressurePlate = false;

public:

	/** Kills the player and triggers respawn at the active checkpoint */
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void KillPlayer();

	/** Teleports the player to the active checkpoint and restores normal status */
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void RespawnAtCheckpoint();

	/** Restores normal player scale (1.0x), normal walk speed, and normal jump */
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void ResetPlayerScaleAndMovement();

	/** Saves a new checkpoint transform */
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void SetActiveCheckpoint(const FTransform& NewCheckpoint);

	/** Returns whether the player has activated at least one checkpoint */
	UFUNCTION(BlueprintCallable, Category = "Checkpoint")
	bool HasActiveCheckpoint() const { return bHasActiveCheckpoint; }

	/** Returns the first person mesh **/
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }

	/** Returns first person camera component **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

};

