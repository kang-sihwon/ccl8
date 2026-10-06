#include "CCLPlayerController.h"

#include "CCLCharacter.h"
#include "CCLGameModeBase.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

// 부모 인터페이스 함수

void ACCLPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(InputComponent);
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (!Subsystem || InputMapping)
	{
		return;
	}
	InputMapping = NewObject<UInputMappingContext>(this);
	auto MakeAction = [this](EInputActionValueType Type)
	{
		UInputAction* Action = NewObject<UInputAction>(this);
		Action->ValueType = Type;
		return Action;
	};
	MoveAction = MakeAction(EInputActionValueType::Axis2D);
	MoveAction->AccumulationBehavior = EInputActionAccumulationBehavior::Cumulative;
	LookAction = MakeAction(EInputActionValueType::Axis2D);
	JumpAction = MakeAction(EInputActionValueType::Boolean);
	RetryAction = MakeAction(EInputActionValueType::Boolean);
	DieAction = MakeAction(EInputActionValueType::Boolean);
	auto MapMove = [this](FKey Key, bool bNegate, bool bSwizzle)
	{
		FEnhancedActionKeyMapping& Mapping = InputMapping->MapKey(MoveAction, Key);
		if (bNegate)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(InputMapping));
		}
		if (bSwizzle)
		{
			UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(InputMapping);
			Swizzle->Order = EInputAxisSwizzle::YXZ;
			Mapping.Modifiers.Add(Swizzle);
		}
	};
	MapMove(EKeys::W, false, true);
	MapMove(EKeys::S, true, true);
	MapMove(EKeys::D, false, false);
	MapMove(EKeys::A, true, false);
	InputMapping->MapKey(LookAction, EKeys::Mouse2D);
	InputMapping->MapKey(JumpAction, EKeys::SpaceBar);
	InputMapping->MapKey(RetryAction, EKeys::R);
	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);
	Input->BindAction(JumpAction, ETriggerEvent::Started, this, &ThisClass::StartJump);
	Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &ThisClass::StopJump);
	Input->BindAction(JumpAction, ETriggerEvent::Canceled, this, &ThisClass::StopJump);
	Input->BindAction(RetryAction, ETriggerEvent::Started, this, &ThisClass::CCLRetry);
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	InputMapping->MapKey(DieAction, EKeys::K);
	Input->BindAction(DieAction, ETriggerEvent::Started, this, &ThisClass::CCLDie);
#endif
	Subsystem->AddMappingContext(InputMapping, 0);
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
}

void ACCLPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (auto* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(); Subsystem && InputMapping)
		{
			Subsystem->RemoveMappingContext(InputMapping);
		}
	}
	Super::EndPlay(EndPlayReason);
}

// 내 클래스 함수

void ACCLPlayerController::CCLRetry() {
	ServerRequestRetry();
}

void ACCLPlayerController::CCLDie()
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	ServerRequestDebugDeath();
#endif
}

void ACCLPlayerController::Move(const FInputActionValue& Value)
{
	ACCLCharacter* ControlledCharacter = Cast<ACCLCharacter>(GetPawn());
	if (!ControlledCharacter || ControlledCharacter->IsDead())
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator Yaw(0.f, GetControlRotation().Yaw, 0.f);
	ControlledCharacter->AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Axis.Y);
	ControlledCharacter->AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Axis.X);
}

void ACCLPlayerController::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddYawInput(Axis.X);
	AddPitchInput(-Axis.Y);
}

void ACCLPlayerController::StartJump()
{
	if (ACCLCharacter* ControlledCharacter = Cast<ACCLCharacter>(GetPawn()); ControlledCharacter && !ControlledCharacter->IsDead())
	{
		ControlledCharacter->Jump();
	}
}

void ACCLPlayerController::StopJump()
{
	if (ACCLCharacter* ControlledCharacter = Cast<ACCLCharacter>(GetPawn()))
	{
		ControlledCharacter->StopJumping();
	}
}

void ACCLPlayerController::ServerRequestRetry_Implementation()
{
	if (auto* GameMode = GetWorld()->GetAuthGameMode<ACCLGameModeBase>())
	{
		GameMode->RequestRetry(this);
	}
}

void ACCLPlayerController::ServerRequestDebugDeath_Implementation()
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (ACCLCharacter* ControlledCharacter = Cast<ACCLCharacter>(GetPawn()))
	{
		ControlledCharacter->Die();
	}
#endif
}
