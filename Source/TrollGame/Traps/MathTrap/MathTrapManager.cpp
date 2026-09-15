// Copyright Epic Games, Inc. All Rights Reserved.

#include "MathTrapManager.h"
#include "MathTrapBuzzer.h"
#include "MathTrapExitDoor.h"
#include "TrollGameCharacter.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

AMathTrapManager::AMathTrapManager()
{
	PrimaryActorTick.bCanEverTick = false;

	CurrentAttempt = 1;
	ConsequenceType = EMathTrapConsequence::TeleportToStart;
	LaunchForce = 1200.0f;

	InitializeDefaultEquations();
}

void AMathTrapManager::InitializeDefaultEquations()
{
	EquationPool.Empty();

	// Category A: Order of Operations / BODMAS Traps
	EquationPool.Add(FComplexMathEquation(TEXT("(8 ÷ 2 × (1 + 3) - 14)"), 2));
	EquationPool.Add(FComplexMathEquation(TEXT("(12 - 3 × 2 + 4 ÷ 2 - 6)"), 2));
	EquationPool.Add(FComplexMathEquation(TEXT("(15 ÷ 3 + 2 × 3 - 8)"), 3));
	EquationPool.Add(FComplexMathEquation(TEXT("(20 ÷ 4 × 2 - 3 × 2)"), 4));
	EquationPool.Add(FComplexMathEquation(TEXT("(6 × 2 ÷ 4 + 5 - 7)"), 1));
	EquationPool.Add(FComplexMathEquation(TEXT("(18 ÷ (3 × 2) + 2^2 - 6)"), 1));

	// Category B: Intimidating Algebraic & Square Root Math
	EquationPool.Add(FComplexMathEquation(TEXT("(√64 ÷ 4 + 5 × 0)"), 2));
	EquationPool.Add(FComplexMathEquation(TEXT("(√100 ÷ 2 - 2^2 + 1)"), 2));
	EquationPool.Add(FComplexMathEquation(TEXT("(3^2 - √16 - 2)"), 3));
	EquationPool.Add(FComplexMathEquation(TEXT("(√81 ÷ 3 + 4^0)"), 4));
	EquationPool.Add(FComplexMathEquation(TEXT("(2^3 ÷ 4 + √9 - 4)"), 1));
	EquationPool.Add(FComplexMathEquation(TEXT("(√49 + 3 × 0)"), 7));
	EquationPool.Add(FComplexMathEquation(TEXT("(5 × 2 - √9)"), 7));
}

void AMathTrapManager::BeginPlay()
{
	Super::BeginPlay();

	SetupRoom();
	ResetRoom();
}

void AMathTrapManager::SetupRoom()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// 1. Auto-discover all buzzers in the room if not manually assigned
	if (RoomBuzzers.Num() == 0)
	{
		for (TActorIterator<AMathTrapBuzzer> It(World); It; ++It)
		{
			AMathTrapBuzzer* Buzzer = *It;
			if (Buzzer)
			{
				Buzzer->TrapManager = this;
				RoomBuzzers.Add(Buzzer);
			}
		}
	}
	else
	{
		for (AMathTrapBuzzer* Buzzer : RoomBuzzers)
		{
			if (Buzzer)
			{
				Buzzer->TrapManager = this;
			}
		}
	}

	// Sort buzzers by BuzzerNumber for clean indexing
	RoomBuzzers.Sort([](const AMathTrapBuzzer& A, const AMathTrapBuzzer& B) {
		return A.BuzzerNumber < B.BuzzerNumber;
	});

	// 2. Auto-locate exit door if not assigned
	if (!ExitDoor)
	{
		for (TActorIterator<AMathTrapExitDoor> It(World); It; ++It)
		{
			ExitDoor = *It;
			break;
		}
	}

	// 3. Auto-locate room screen actor if not assigned
	if (!RoomScreenActor)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (!Actor) continue;

			FString Label = Actor->GetActorLabel();
			if (Label.Equals(TEXT("Screen"), ESearchCase::IgnoreCase) || Actor->ActorHasTag(TEXT("Screen")))
			{
				RoomScreenActor = Actor;
				break;
			}
		}
	}

	// 4. Auto-locate room start point if not assigned
	if (!RoomStartPoint)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (!Actor) continue;

			FString Label = Actor->GetActorLabel();
			if (Label.Equals(TEXT("Start"), ESearchCase::IgnoreCase) || 
			    Label.Contains(TEXT("MathRoomStart"), ESearchCase::IgnoreCase) || 
			    Actor->ActorHasTag(TEXT("Start")))
			{
				RoomStartPoint = Actor;
				break;
			}
		}
	}

	// 5. Cache or attach text render component on the screen actor
	if (RoomScreenActor)
	{
		ScreenTextComp = RoomScreenActor->FindComponentByClass<UTextRenderComponent>();
		if (!ScreenTextComp)
		{
			ScreenTextComp = NewObject<UTextRenderComponent>(RoomScreenActor, TEXT("MathRoomScreenText"));
			if (ScreenTextComp)
			{
				ScreenTextComp->RegisterComponent();
				ScreenTextComp->AttachToComponent(RoomScreenActor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
				ScreenTextComp->SetAbsolute(false, false, true);
				ScreenTextComp->SetWorldScale3D(FVector(1.0f, 1.0f, 1.0f));
				ScreenTextComp->SetRelativeLocation(FVector(0.0f, 55.0f, 0.0f));
				ScreenTextComp->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
				ScreenTextComp->SetWorldSize(34.0f);
				ScreenTextComp->SetHorizSpacingAdjust(1.0f);
				ScreenTextComp->SetVertSpacingAdjust(1.25f);
				ScreenTextComp->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
				ScreenTextComp->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
			}
		}
	}
}

void AMathTrapManager::ResetRoom()
{
	CurrentAttempt = 1;

	if (FakeHighlightedBuzzer)
	{
		FakeHighlightedBuzzer->SetHighlight(false);
		FakeHighlightedBuzzer = nullptr;
	}

	if (ExitDoor)
	{
		ExitDoor->ResetDoor();
	}

	PickNewEquation();

	// Attempt 1: Raw equation only - player has NO clue about tricks
	FString WelcomeText = FString::Printf(
		TEXT("DOOR LOCKED: SOLVE TO ESCAPE\n\n%s = ?\n\nPress the matching numbered buzzer (1 - 10)!"),
		*CurrentEquation.EquationText
	);

	UpdateScreenText(WelcomeText, FColor(0, 255, 235, 255)); // Neon Cyan
}

void AMathTrapManager::PickNewEquation()
{
	if (EquationPool.Num() == 0)
	{
		InitializeDefaultEquations();
	}

	// Refill and shuffle pool when exhausted to prevent repeats
	if (UnusedEquationIndices.Num() == 0)
	{
		for (int32 i = 0; i < EquationPool.Num(); ++i)
		{
			UnusedEquationIndices.Add(i);
		}

		// Fisher-Yates shuffle
		for (int32 i = UnusedEquationIndices.Num() - 1; i > 0; --i)
		{
			int32 SwapIdx = FMath::RandRange(0, i);
			UnusedEquationIndices.Swap(i, SwapIdx);
		}
	}

	int32 SelectedIdx = UnusedEquationIndices.Pop();
	CurrentEquation = EquationPool[SelectedIdx];
}

void AMathTrapManager::OnBuzzerPressed(AMathTrapBuzzer* PressedBuzzer, ACharacter* PlayerCharacter)
{
	if (!PressedBuzzer) return;

	int32 PressedNum = PressedBuzzer->BuzzerNumber;
	int32 A = CurrentEquation.BaseAnswer;
	int32 SummationAns = A + A;
	int32 Attempt3Ans = SummationAns + SummationAns; // e.g., 4th buzzer from buzzer 4 = 8

	// If the door is already unlocked and open, ignore further buzzers
	if (ExitDoor && ExitDoor->bIsOpen)
	{
		return;
	}

	switch (CurrentAttempt)
	{
	case 1:
	{
		// Attempt 1: Wrong! Reveal the summation hint for the first time
		if (WrongBuzzerSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, WrongBuzzerSound, PressedBuzzer->GetActorLocation());
		}

		ApplyConsequence(PlayerCharacter, PressedBuzzer);

		CurrentAttempt = 2;

		FString Hint2Text = FString::Printf(
			TEXT("DID YOU REALLY THINK IT WAS THAT EASY?\n\nNo one escapes on the first try!\nA true mathematician knows you must do the SUMMATION of the answer!\n\n(e.g., If the answer is %d, you must do %d + %d = %d)\n\nNow go press buzzer #%d!"),
			A, A, A, SummationAns, SummationAns
		);

		UpdateScreenText(Hint2Text, FColor(255, 200, 0, 255)); // Amber Warning
		break;
	}

	case 2:
	{
		// Attempt 2: Wrong! Developer forgot to write the rest of the hint
		if (WrongBuzzerSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, WrongBuzzerSound, PressedBuzzer->GetActorLocation());
		}

		ApplyConsequence(PlayerCharacter, PressedBuzzer);

		CurrentAttempt = 3;

		FString Hint3Text = FString::Printf(
			TEXT("DEV OOPSIE!\n\nThe developer forgot to write the entire hint!\nHere is the missing part:\n\n'If the answer is %d, press the %dth buzzer from the %dth labeled buzzer!'\n\n(Hurry, press buzzer #%d!)"),
			SummationAns, SummationAns, SummationAns, Attempt3Ans
		);

		UpdateScreenText(Hint3Text, FColor(255, 120, 0, 255)); // Orange
		break;
	}

	case 3:
	{
		// Attempt 3: Wrong! Roast the player's intelligence
		if (WrongBuzzerSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, WrongBuzzerSound, PressedBuzzer->GetActorLocation());
		}

		ApplyConsequence(PlayerCharacter, PressedBuzzer);

		CurrentAttempt = 4;

		FString RoastText = TEXT("ARE YOU SERIOUS?!\n\nEven a calculator from 1980 has better logic than you!\n\nYou're just blindly following random hints on a screen!\nStop guessing and use your brain!");

		UpdateScreenText(RoastText, FColor(255, 50, 50, 255)); // Red Roast
		break;
	}

	case 4:
	{
		// Attempt 4: Player failed again after the roast. Developer offers fake help
		if (WrongBuzzerSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, WrongBuzzerSound, PressedBuzzer->GetActorLocation());
		}

		ApplyConsequence(PlayerCharacter, PressedBuzzer);

		CurrentAttempt = 5;

		// Pick a fake buzzer to highlight that is NOT the true answer
		FakeHighlightedBuzzer = nullptr;
		for (AMathTrapBuzzer* B : RoomBuzzers)
		{
			if (B && B->BuzzerNumber != A)
			{
				FakeHighlightedBuzzer = B;
				break;
			}
		}

		if (FakeHighlightedBuzzer)
		{
			FakeHighlightedBuzzer->SetHighlight(true);
		}

		FString FakeHelpText = TEXT("DEV APOLOGY:\n\nOkay, look... the developer felt bad for you.\n\nI highlighted the correct buzzer with a bright glowing green light so you can just escape already.\n\nGo press the glowing green buzzer!");

		UpdateScreenText(FakeHelpText, FColor(50, 255, 100, 255)); // Green Deception
		break;
	}

	case 5:
	{
		// Attempt 5: Player presses the fake highlighted buzzer (or another buzzer)
		if (FakeHighlightedBuzzer)
		{
			FakeHighlightedBuzzer->SetHighlight(false);
			FakeHighlightedBuzzer = nullptr;
		}

		if (PrankLaughSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, PrankLaughSound, PressedBuzzer->GetActorLocation());
		}
		else if (WrongBuzzerSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, WrongBuzzerSound, PressedBuzzer->GetActorLocation());
		}

		ApplyConsequence(PlayerCharacter, PressedBuzzer);

		CurrentAttempt = 6;

		FString FooledYouText = FString::Printf(
			TEXT("FOOLED YOU! HAHAHA!\n\nThere are no shortcuts in \"WHOOPS!\"\n\nNow stop looking for cheats and DO MATH PROPERLY!\n\nSolve: %s\nPress the TRUE answer buzzer to unlock the door!"),
			*CurrentEquation.EquationText
		);

		UpdateScreenText(FooledYouText, FColor(255, 0, 200, 255)); // Neon Magenta
		break;
	}

	default: // Attempt 6 and beyond
	{
		if (PressedNum == A)
		{
			// SUCCESS! True math answer entered!
			if (VictorySound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, VictorySound, PressedBuzzer->GetActorLocation());
			}

			if (ExitDoor)
			{
				ExitDoor->UnlockAndOpen();
			}

			FString WinText = TEXT("FINALLY!\n\nYou actually did math properly!\nThe exit door is now unlocked.\n\nDon't let the door hit you on the way out!");
			UpdateScreenText(WinText, FColor(0, 255, 100, 255)); // Bright Green Victory
		}
		else
		{
			// WRONG BUZZER ON ATTEMPT 6+
			if (WrongBuzzerSound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, WrongBuzzerSound, PressedBuzzer->GetActorLocation());
			}

			ApplyConsequence(PlayerCharacter, PressedBuzzer);

			FString KeepTryingText = FString::Printf(
				TEXT("WRONG BUZZER!\n\nDo math properly! The equation is:\n\n%s = ?\n\nFind the true answer!"),
				*CurrentEquation.EquationText
			);

			UpdateScreenText(KeepTryingText, FColor(255, 50, 50, 255));
		}
		break;
	}
	}
}

void AMathTrapManager::ApplyConsequence(ACharacter* PlayerChar, AMathTrapBuzzer* FromBuzzer)
{
	if (!PlayerChar) return;

	switch (ConsequenceType)
	{
	case EMathTrapConsequence::TeleportToStart:
	{
		if (RoomStartPoint)
		{
			FVector TargetLoc = RoomStartPoint->GetActorLocation();
			FRotator TargetRot = RoomStartPoint->GetActorRotation();
			PlayerChar->TeleportTo(TargetLoc, TargetRot, false, true);
		}
		else
		{
			// Teleport slightly back from the buzzer
			FVector SafeLoc = PlayerChar->GetActorLocation() - PlayerChar->GetActorForwardVector() * 300.0f;
			PlayerChar->TeleportTo(SafeLoc, PlayerChar->GetActorRotation(), false, true);
		}

		if (PlayerChar->GetCharacterMovement())
		{
			PlayerChar->GetCharacterMovement()->Velocity = FVector::ZeroVector;
		}
		break;
	}

	case EMathTrapConsequence::LaunchBackwards:
	{
		FVector LaunchDir = (PlayerChar->GetActorLocation() - FromBuzzer->GetActorLocation()).GetSafeNormal();
		LaunchDir.Z = 0.45f;
		LaunchDir.Normalize();

		PlayerChar->LaunchCharacter(LaunchDir * LaunchForce, true, true);
		break;
	}

	case EMathTrapConsequence::KillPlayer:
	{
		if (ATrollGameCharacter* TrollChar = Cast<ATrollGameCharacter>(PlayerChar))
		{
			TrollChar->KillPlayer();
		}
		break;
	}
	}
}

void AMathTrapManager::UpdateScreenText(const FString& NewText, const FColor& TextColor)
{
	if (ScreenTextComp)
	{
		ScreenTextComp->SetText(FText::FromString(NewText));
		ScreenTextComp->SetTextRenderColor(TextColor);
	}
}
