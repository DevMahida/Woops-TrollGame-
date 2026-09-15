// Copyright Epic Games, Inc. All Rights Reserved.

#include "MathTrapBuzzer.h"
#include "MathTrapManager.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/AudioComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

AMathTrapBuzzer::AMathTrapBuzzer()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	BaseMesh->SetupAttachment(RootComponent);
	BaseMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	ButtonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ButtonMesh"));
	ButtonMesh->SetupAttachment(RootComponent);
	ButtonMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 20.0f));
	ButtonMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	NumberText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NumberText"));
	NumberText->SetupAttachment(RootComponent);
	NumberText->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
	NumberText->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	NumberText->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	NumberText->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	NumberText->SetWorldSize(32.0f);
	NumberText->SetTextRenderColor(FColor(255, 220, 50, 255)); // Warm gold

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(RootComponent);
	TriggerBox->SetBoxExtent(FVector(50.0f, 50.0f, 40.0f));
	TriggerBox->SetRelativeLocation(FVector(0.0f, 0.0f, 30.0f));
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
	TriggerBox->SetGenerateOverlapEvents(true);

	HighlightLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("HighlightLight"));
	HighlightLight->SetupAttachment(ButtonMesh);
	HighlightLight->SetRelativeLocation(FVector(0.0f, 0.0f, 20.0f));
	HighlightLight->SetLightColor(FLinearColor(0.05f, 1.0f, 0.2f)); // Neon green
	HighlightLight->SetIntensity(3000.0f);
	HighlightLight->SetAttenuationRadius(250.0f);
	HighlightLight->SetVisibility(false);

	AudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComp"));
	AudioComp->SetupAttachment(RootComponent);
	AudioComp->bAutoActivate = false;

	BuzzerNumber = 1;
	DepressDistance = 5.0f;
	AnimationSpeed = 15.0f;
	bCanBePressed = true;
	bIsHighlighted = false;
}

void AMathTrapBuzzer::BeginPlay()
{
	Super::BeginPlay();

	if (ButtonMesh)
	{
		ButtonRestLocation = ButtonMesh->GetRelativeLocation();
		ButtonTargetLocation = ButtonRestLocation - FVector(0.0f, 0.0f, DepressDistance);
	}

	if (NumberText)
	{
		NumberText->SetText(FText::AsNumber(BuzzerNumber));
	}

	if (TriggerBox)
	{
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AMathTrapBuzzer::OnTriggerOverlap);
	}

	// Auto-locate manager if not manually assigned
	if (!TrapManager && GetWorld())
	{
		for (TActorIterator<AMathTrapManager> It(GetWorld()); It; ++It)
		{
			TrapManager = *It;
			break;
		}
	}
}

void AMathTrapBuzzer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsAnimating || !ButtonMesh)
	{
		return;
	}

	FVector CurrentLoc = ButtonMesh->GetRelativeLocation();

	if (!bIsReturning)
	{
		// Moving downwards
		FVector NewLoc = FMath::VInterpTo(CurrentLoc, ButtonTargetLocation, DeltaTime, AnimationSpeed);
		ButtonMesh->SetRelativeLocation(NewLoc);

		if (FVector::DistSquared(NewLoc, ButtonTargetLocation) < 0.25f)
		{
			bIsReturning = true;
		}
	}
	else
	{
		// Returning to rest
		FVector NewLoc = FMath::VInterpTo(CurrentLoc, ButtonRestLocation, DeltaTime, AnimationSpeed);
		ButtonMesh->SetRelativeLocation(NewLoc);

		if (FVector::DistSquared(NewLoc, ButtonRestLocation) < 0.25f)
		{
			ButtonMesh->SetRelativeLocation(ButtonRestLocation);
			bIsAnimating = false;
			bIsReturning = false;
		}
	}
}

void AMathTrapBuzzer::OnTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bCanBePressed || !OtherActor || OtherActor == this)
	{
		return;
	}

	ACharacter* PlayerChar = Cast<ACharacter>(OtherActor);
	if (PlayerChar)
	{
		PressBuzzer(PlayerChar);
	}
}

void AMathTrapBuzzer::PressBuzzer(ACharacter* PlayerCharacter)
{
	if (!bCanBePressed)
	{
		return;
	}

	bCanBePressed = false;
	bIsAnimating = true;
	bIsReturning = false;

	if (PressSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PressSound, GetActorLocation());
	}

	// 1 second debounce
	GetWorldTimerManager().SetTimer(CooldownTimerHandle, this, &AMathTrapBuzzer::ResetCooldown, 1.0f, false);

	if (TrapManager)
	{
		TrapManager->OnBuzzerPressed(this, PlayerCharacter);
	}
}

void AMathTrapBuzzer::SetHighlight(bool bEnable)
{
	bIsHighlighted = bEnable;
	if (HighlightLight)
	{
		HighlightLight->SetVisibility(bEnable);
	}

	if (NumberText)
	{
		// Switch text color to match highlight
		NumberText->SetTextRenderColor(bEnable ? FColor(50, 255, 100, 255) : FColor(255, 220, 50, 255));
	}
}

void AMathTrapBuzzer::ResetCooldown()
{
	bCanBePressed = true;
}
