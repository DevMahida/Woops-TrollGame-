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
	FString RoomShrinkTrapMessage = TEXT("HONEY, I SHRUNK THE NOOB!\n\nNow your body matches your skill level.\nGood luck making that jump with those tiny legs!\n\n[Hint: The middle is a trap! Try checking the side panels...]\n\n[Size: Micro | Skill Level: Noob]");

	/** Message displayed on the room screen when player steps on a side pressure plate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Screen")
	FString RoomSidePlateShrinkMessage = TEXT("LMAO! You really thought you cooked\nby walking on the side panel?\n\nAww, sad life for you!\nYour body is now officially as tiny as your brain!\n\n[Hint: That side panel failed! Maybe try the OTHER side panel?]\n\n[Size: Ant-Size | Skill Level: Trash]");

	/** Message displayed when shrunk player steps on side pressure plate after getting shrunk on main plate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Screen")
	FString RoomShrunkMainThenSideMessage = TEXT("NICE TRY, TINY BRAIN! 😂\n\nYou already got shrunk and thought\n\"Let me sneak around the side\"?\n\nAww, how cute! But you're still dumb\nbecause of your micro-sized brain!\n\n[Hint: Still stuck? There is more than one side panel in this room!]\n\n[Tactic: Avoidance | IQ: 0 | Size: Micro]");

	/** Message displayed when shrunk player steps on side pressure plate again after already getting shrunk on side plate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Screen")
	FString RoomShrunkSideRepeatMessage = TEXT("YOU'RE TINY AND AN IDIOT! 😂\n\nYou already know what happens here!\nWherever you walk, it doesn't matter—\nyou can't escape the dev's traps!\n\n[Hint: Insanity is doing the same thing twice!\nTry the OPPOSITE side panel or find another angle!]\n\n[Status: Repeat Offender | Brain: Absent]");

	/** Message displayed when shrunk player steps on main plate after getting shrunk on side plate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Screen")
	FString RoomShrunkSideThenMainMessage = TEXT("BACK TO THE MAIN PLATE? 😂\n\nFirst you tried the side, now the middle?\nYou really love stepping on every\nsingle trap in this room, don't you?\n\n[Hint: The middle is ALWAYS a trap!\nGo explore the OTHER side panel instead!]\n\n[Trap Completion: 100% | Skill: 0%]");

	/** Message displayed when shrunk player steps on main plate again */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Screen")
	FString RoomShrunkMainRepeatMessage = TEXT("STILL STEPPING IN THE MIDDLE?!\n\nYou already got shrunk by this exact plate!\nDid you think it would grow you back?\n\n[Hint: The middle plate won't heal you!\nCheck the side panels around the room!]\n\n[Status: Stubborn | Learning Curve: Flat]");

	/** Message displayed when side pressure plates are disabled after enough tries */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Screen")
	FString RoomSidePlatesDisabled6thMessage = TEXT("OKAY FINE! WE GIVE UP! 😂\n\nWe feel so bad watching you struggle...\nWe've made some subtle changes so you can\nfinally move on towards the 1st level.\n\nGo ahead, give it another try!\nYou're welcome, noob!\n\n[Status: Nerfed For You | Mode: Pity Unlocked]");

	/** Message displayed when player presses the same plate repeatedly (reducing speed) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Screen")
	FString RoomRepeatPlatePunishMessage = TEXT("STOP PRESSING THE SAME PLATE! 🐌\n\nDid you think doing it again would give a different result?\nAs a punishment, your walk speed has been reduced even more!\n\n[Penalty: Sloth Speed | Brain: Offline]");

	/** Source of current shrink: 0 = None, 1 = MainPlate, 2 = SidePlate */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	int32 LastShrinkSourcePlate = 0;

	/** Total count of pressure plate triggers in current run */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	int32 PressurePlateTriggerCount = 0;

	/** Number of consecutive times the same plate type was pressed */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	int32 ConsecutiveSamePlateCount = 0;

	/** Whether side pressure plates are disabled after 6 triggers */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trap|PressurePlate")
	bool bSidePressurePlatesDisabled = false;

	/** Hint message on 1st void fall while shrunk */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Screen")
	FString ShrunkVoidFallHint1Message = TEXT("HEY TINY NOOB!\n\nStruggling with the hallway trap?\n\nHint: You might wanna try the other side panel...\nMaybe it's safer? 😉");

	/** Message on 2nd void fall while shrunk */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Screen")
	FString ShrunkVoidFallHint2Message = TEXT("FELL AGAIN ALREADY?!\n\nDid you really think that would work?\n\nThose tiny legs aren't helping much, are they?\nKeep trying!");

	/** Message on 3rd void fall while shrunk (disables the push trap) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Screen")
	FString ShrunkVoidFallHint3Message = TEXT("YOU SERIOUSLY THOUGHT WE WOULD LET YOU GO\nTHAT EASILY WITHOUT HAVING FUN WITH YOU? 😂\n\nAlright, fine! We've had enough fun.\nThe side pushers are now disabled.\nGo ahead, walk through!");

	/** Random troll messages displayed on the room screen when a shrunk player falls into the void */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Screen")
	TArray<FString> ShrunkVoidFallMessages;

	/** Number of times player has fallen into the void while shrunk */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	int32 ShrunkVoidFallCount = 0;

	/** Whether the side hallway pusher trap has been disabled after 3 shrunk void falls */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trap|HallwayPush")
	bool bHallwayPushTrapDisabled = false;

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

	/** Cached pointer to HallwayPush actor 1 */
	UPROPERTY(Transient)
	TObjectPtr<AActor> HallwayPushActor1;

	/** Cached pointer to HallwayPush2 actor 2 */
	UPROPERTY(Transient)
	TObjectPtr<AActor> HallwayPushActor2;

	/** Cached pointer to ComeNear target actor */
	UPROPERTY(Transient)
	TObjectPtr<AActor> ComeNearActor;

	/** Initial starting transform location of HallwayPush 1 */
	UPROPERTY(Transient)
	FVector Push1InitialLoc;

	/** Initial starting transform location of HallwayPush 2 */
	UPROPERTY(Transient)
	FVector Push2InitialLoc;

	/** Whether 0.5s delay timer is active for HallwayPush trap */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trap|HallwayPush")
	bool bHallwayPushDelayActive = false;

	/** Whether HallwayPush actors are currently actively moving towards each other */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trap|HallwayPush")
	bool bHallwayPushMoving = false;

	/** Accumulated timer for 0.5s delay */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trap|HallwayPush")
	float HallwayPushDelayTimer = 0.0f;

	/** Delay duration before pushers activate (0.5 seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|HallwayPush")
	float HallwayPushDelayDuration = 0.5f;

	/** Movement speed of HallwayPush actors towards ComeNear (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|HallwayPush")
	float HallwayPushSpeed = 1000.0f;

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

