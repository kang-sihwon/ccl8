#include "CCLCombatAbility.h"

#include "CCLAbilitySystemComponent.h"
#include "CCLEffects.h"
#include "CCLGameplayTags.h"
#include "CCLStaminaSet.h"
#include "Combat/CCLCombatComponent.h"
#include "Combat/CCLCombatDefinition.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UCCLCombatAbility::UCCLCombatAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ActivationBlockedTags.AddTag(CCLTags::State_Busy);
	ActivationBlockedTags.AddTag(CCLTags::State_Dead);
	ActivationBlockedTags.AddTag(CCLTags::State_Stagger);
}

bool UCCLCombatAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	const auto* Component = Avatar ? Avatar->FindComponentByClass<UCCLFighterComponent>() : nullptr;
	const auto* Definition = Component ? Component->GetAttack() : nullptr;

	if (!Definition || !FMath::IsFinite(Definition->Damage) || Definition->Damage < 0.f ||
	    !FMath::IsFinite(Definition->Cost) || Definition->Cost < 0.f ||
	    Definition->Windup < 0.f || Definition->Active <= 0.f || Definition->Recovery < 0.f ||
	    Definition->Reach <= 0.f || Definition->Radius <= 0.f || !Definition->DamageEffect || !Definition->HitRule ||
	    Component->DodgeMoveDuration <= 0.f || Component->DodgeDuration < Component->InvulnerableEnd ||
	    Component->ParryDuration < Component->ParryEnd)
	{
		return false;
	}

	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

bool UCCLCombatAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	const float Cost = GetCost(ActorInfo);
	return Cost <= 0.f || (ActorInfo && ActorInfo->AbilitySystemComponent->HasAttributeSetForAttribute(UCCLStaminaSet::GetStaminaAttribute()) &&
		ActorInfo->AbilitySystemComponent->GetNumericAttribute(UCCLStaminaSet::GetStaminaAttribute()) >= Cost);
}

void UCCLCombatAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const float Cost = GetCost(ActorInfo);

	if (Cost <= 0.f)
	{
		return;
	}

	auto Spec = ActorInfo->AbilitySystemComponent->MakeOutgoingSpec(UCCLStaminaChangeEffect::StaticClass(), 1.f, ActorInfo->AbilitySystemComponent->MakeEffectContext());
	Spec.Data->SetSetByCallerMagnitude(CCLTags::Data_Magnitude, -Cost);
	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
}

void UCCLCombatAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ActivatedAvatar = ActorInfo->AvatarActor.Get();
	Fighter = ActivatedAvatar->FindComponentByClass<UCCLFighterComponent>();

	if (!Fighter || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	auto* ASC = CastChecked<UCCLAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get());
	BusyEffect = ASC->ApplyEffect(UCCLBusyEffect::StaticClass());
	Fighter->SetAction(Action);
	const UCCLCombatDefinition* Attack = Fighter->GetAttack();
	float Start = 0.f;
	float End = 0.f;
	float Duration = 0.f;
	UAnimMontage* Montage = nullptr;

	if (Action == ECCLCombatAction::Attack)
	{
		Start = Attack->Windup;
		End = Start + Attack->Active;
		Duration = End + Attack->Recovery;
		Montage = Attack->Montage;
	}
	else if (Action == ECCLCombatAction::Dodge)
	{
		Start = Fighter->InvulnerableStart;
		End = Fighter->InvulnerableEnd;
		Duration = Fighter->DodgeDuration;
		Montage = Fighter->DodgeMontage;

		if (auto* Character = Cast<ACharacter>(ActivatedAvatar.Get()))
		{
			FVector Direction = Character->GetCharacterMovement()->GetCurrentAcceleration().GetSafeNormal2D();

			if (Direction.IsNearlyZero())
			{
				Direction = -Character->GetActorForwardVector();
			}

			auto* Move = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(this, TEXT("Dodge"), Direction,
			    Fighter->DodgeDistance / Fighter->DodgeMoveDuration, Fighter->DodgeMoveDuration, false, nullptr,
			    ERootMotionFinishVelocityMode::SetVelocity, FVector::ZeroVector, 0.f, true);
			Move->ReadyForActivation();
		}
	}
	else if (Action == ECCLCombatAction::Parry)
	{
		Montage = Fighter->DefenseMontage;
		Start = Fighter->ParryStart;
		End = Fighter->ParryEnd;
		Duration = Fighter->ParryDuration;
	}
	else if (Action == ECCLCombatAction::Guard)
	{
		WindowEffect = ASC->ApplyEffect(UCCLGuardEffect::StaticClass());

		if (Fighter->DefenseMontage)
		{
			auto* Pose = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Fighter->DefenseMontage);
			Pose->ReadyForActivation();
		}

		auto* ReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true);
		ReleaseTask->OnRelease.AddDynamic(this, &ThisClass::Released);
		ReleaseTask->ReadyForActivation();
		return;
	}

	if (Montage)
	{
		auto* Play = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage, Action == ECCLCombatAction::Parry ? 1.f : Montage->GetPlayLength() / Duration);
		Play->ReadyForActivation();
	}

	auto* Open = UAbilityTask_WaitDelay::WaitDelay(this, Start);
	Open->OnFinish.AddDynamic(this, &ThisClass::BeginWindow);
	Open->ReadyForActivation();
	auto* Close = UAbilityTask_WaitDelay::WaitDelay(this, End);
	Close->OnFinish.AddDynamic(this, &ThisClass::EndWindow);
	Close->ReadyForActivation();
	auto* Complete = UAbilityTask_WaitDelay::WaitDelay(this, Duration);
	Complete->OnFinish.AddDynamic(this, &ThisClass::Finish);
	Complete->ReadyForActivation();
}

void UCCLCombatAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (HasCurrentAvatar())
	{
		EndWindow();
		Fighter->SetAction(ECCLCombatAction::None);
		ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(BusyEffect);
	}

	BusyEffect.Invalidate();
	WindowEffect.Invalidate();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	Fighter = nullptr;
	ActivatedAvatar.Reset();
}

float UCCLCombatAbility::GetCost(const FGameplayAbilityActorInfo* ActorInfo) const
{
	const auto* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	const auto* Component = Avatar ? Avatar->FindComponentByClass<UCCLFighterComponent>() : nullptr;

	if (!Component)
	{
		return 0.f;
	}

	if (Action == ECCLCombatAction::Attack)
	{
		return Component->GetAttack() ? Component->GetAttack()->Cost : 0.f;
	}

	if (Action == ECCLCombatAction::Dodge)
	{
		return Component->DodgeCost;
	}

	if (Action == ECCLCombatAction::Parry)
	{
		return Component->ParryCost;
	}

	return 0.f;
}

bool UCCLCombatAbility::HasCurrentAvatar() const
{
	return Fighter && ActivatedAvatar.IsValid() && CurrentActorInfo && CurrentActorInfo->AvatarActor == ActivatedAvatar;
}

void UCCLCombatAbility::BeginWindow()
{
	if (!HasCurrentAvatar())
	{
		return;
	}

	auto* ASC = Fighter->GetASC();

	if (Action == ECCLCombatAction::Attack)
	{
		Fighter->LockAttackDirection();

		if (auto* Combat = ActivatedAvatar->FindComponentByClass<UCCLCombatComponent>())
		{
			Combat->BeginAttack(Fighter->GetAttack(), ActivatedAvatar->GetActorForwardVector());
		}
	}
	else if (Action == ECCLCombatAction::Dodge)
	{
		WindowEffect = ASC->ApplyEffect(UCCLInvulnerableEffect::StaticClass());
	}
	else if (Action == ECCLCombatAction::Parry)
	{
		WindowEffect = ASC->ApplyEffect(UCCLParryEffect::StaticClass());
	}
}

void UCCLCombatAbility::EndWindow()
{
	if (!HasCurrentAvatar())
	{
		return;
	}

	if (auto* Combat = ActivatedAvatar->FindComponentByClass<UCCLCombatComponent>(); Combat && Action == ECCLCombatAction::Attack)
	{
		Combat->EndAttack();
	}

	if (WindowEffect.IsValid())
	{
		Fighter->GetASC()->RemoveActiveGameplayEffect(WindowEffect);
	}

	WindowEffect.Invalidate();
}

void UCCLCombatAbility::Finish()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UCCLCombatAbility::Released(float TimeHeld)
{
	Finish();
}

UCCLAttackAbility::UCCLAttackAbility()
{
	Action = ECCLCombatAction::Attack;
}

UCCLDodgeAbility::UCCLDodgeAbility()
{
	Action = ECCLCombatAction::Dodge;
}

UCCLGuardAbility::UCCLGuardAbility()
{
	Action = ECCLCombatAction::Guard;
}

UCCLParryAbility::UCCLParryAbility()
{
	Action = ECCLCombatAction::Parry;
}
