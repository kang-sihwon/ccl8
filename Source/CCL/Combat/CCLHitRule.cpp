#include "CCLHitRule.h"

#include "CCLCombatDefinition.h"
#include "CCLFighterComponent.h"
#include "AbilitySystem/CCLAbilitySystemComponent.h"
#include "AbilitySystem/CCLEffects.h"
#include "AbilitySystem/CCLGameplayTags.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "AbilitySystem/CCLOffenseSet.h"
#include "AbilitySystem/CCLStaminaSet.h"

namespace
{
	FCCLHitResolution Reject()
	{
		return FCCLHitResolution(FGameplayTag(CCLTags::Outcome_Rejected));
	}

	FCCLHitResolution Damage(const FCCLHitContext& Context)
	{
		const bool bBonus = !Context.bDetachedShot && Context.SourceASC && Context.SourceASC->GetSet<UCCLOffenseSet>();
		const float Bonus = bBonus ? Context.SourceASC->GetNumericAttribute(UCCLOffenseSet::GetAttackBonusAttribute()) : 0.f;
		return {CCLTags::Outcome_Damage, Context.Definition->DamageEffect, Context.Definition->MagnitudeTag, -(Context.Definition->Damage + Bonus)};
	}
}

// 내 클래스 함수

FCCLHitResolution UCCLHitRule::Resolve(const FCCLHitContext& Context) const
{
	return {};
}

void UCCLHitRule::Commit(const FCCLHitContext& Context, const FCCLHitResolution& Resolution) const
{
}

// 부모 인터페이스 함수

FCCLHitResolution UCCLDuelHitRule::Resolve(const FCCLHitContext& Context) const
{
	if ((!Context.bDetachedShot && Context.SourceASC->HasMatchingGameplayTag(CCLTags::State_Dead)) || Context.TargetASC->HasMatchingGameplayTag(CCLTags::State_Dead))
	{
		return Reject();
	}

	if (!Context.TargetASC->HasAttributeSetForAttribute(UCCLHealthSet::GetHealthAttribute()) || Context.TargetASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) <= 0.f)
	{
		return Reject();
	}

	const auto* SourceFighter = Context.Source->FindComponentByClass<UCCLFighterComponent>();
	const auto* TargetFighter = Context.Target->FindComponentByClass<UCCLFighterComponent>();
	const int32 SourceTeam = Context.bDetachedShot ? Context.CapturedTeam : (SourceFighter ? SourceFighter->Team : INDEX_NONE);
	if (SourceTeam != INDEX_NONE && TargetFighter && SourceTeam == TargetFighter->Team)
	{
		return Reject();
	}

	if (Context.TargetASC->HasMatchingGameplayTag(CCLTags::State_Invulnerable))
	{
		return FCCLHitResolution(FGameplayTag(CCLTags::Outcome_Evaded));
	}

	if (!TargetFighter)
	{
		return Damage(Context);
	}

	if (!Cast<UCCLAbilitySystemComponent>(Context.TargetASC))
	{
		return Reject();
	}

	const FVector ToAttacker = Context.bDetachedShot ? -Context.IncomingDirection.GetSafeNormal2D() :
		(Context.Source->GetActorLocation() - Context.Target->GetActorLocation()).GetSafeNormal2D();
	const bool bFrontal = FVector::DotProduct(Context.Target->GetActorForwardVector(), ToAttacker) >= FMath::Cos(FMath::DegreesToRadians(TargetFighter->DefenseAngle * 0.5f));

	if (bFrontal && Context.Definition->bParryable && Context.TargetASC->HasMatchingGameplayTag(CCLTags::State_Parry))
	{
		return FCCLHitResolution(FGameplayTag(CCLTags::Outcome_Parried));
	}

	if (bFrontal && Context.Definition->bGuardable && Context.TargetASC->HasMatchingGameplayTag(CCLTags::State_Guard) && Context.TargetASC->GetSet<UCCLStaminaSet>())
	{
		const float Stamina = Context.TargetASC->GetNumericAttribute(UCCLStaminaSet::GetStaminaAttribute());
		return FCCLHitResolution(FGameplayTag(Stamina <= TargetFighter->GuardCost ? CCLTags::Outcome_GuardBroken : CCLTags::Outcome_Guarded));
	}

	return Damage(Context);
}

void UCCLDuelHitRule::Commit(const FCCLHitContext& Context, const FCCLHitResolution& Resolution) const
{
	auto* TargetFighter = Context.Target->FindComponentByClass<UCCLFighterComponent>();
	auto* TargetASC = Cast<UCCLAbilitySystemComponent>(Context.TargetASC);
	if (!TargetFighter || !TargetASC)
	{
		return;
	}

	if (Resolution.Outcome == CCLTags::Outcome_Parried)
	{
		if (!Context.bDetachedShot && Context.SourceASC)
		{
			Context.SourceASC->CancelAllAbilities();
			if (auto* SourceASC = Cast<UCCLAbilitySystemComponent>(Context.SourceASC))
			{
				SourceASC->ApplyEffect(UCCLStaggerEffect::StaticClass(), 0.f, TargetFighter->ParryStaggerDuration);
			}
		}

		TargetFighter->NotifyHit(Resolution.Outcome);
	}
	else if (Resolution.Outcome == CCLTags::Outcome_Guarded || Resolution.Outcome == CCLTags::Outcome_GuardBroken)
	{
		TargetASC->ApplyEffect(UCCLStaminaChangeEffect::StaticClass(), -TargetFighter->GuardCost);
		if (Resolution.Outcome == CCLTags::Outcome_GuardBroken)
		{
			TargetASC->CancelAllAbilities();
			TargetASC->ApplyEffect(UCCLStaggerEffect::StaticClass(), 0.f, TargetFighter->GuardBreakDuration);
		}

		TargetFighter->NotifyHit(Resolution.Outcome);
	}
	else if (Resolution.Outcome == CCLTags::Outcome_Damage)
	{
		TargetASC->CancelAllAbilities();
		TargetFighter->NotifyHit(Resolution.Outcome);
	}
}

// 판정 진입점

FGameplayTag CCLHit::Apply(const FCCLHitContext& Context)
{
	if (!IsValid(Context.Source) || !IsValid(Context.Target) || !Context.Source->HasAuthority() ||
		Context.Source == Context.Target || (!Context.SourceASC && !(Context.bDetachedShot && Context.CapturedEffect.IsValid())) || !Context.TargetASC ||
		Context.TargetASC->GetAvatarActor() != Context.Target || !Context.Definition || !Context.Definition->HitRule)
	{
		return {};
	}

	const auto* Rule = Context.Definition->HitRule->GetDefaultObject<UCCLHitRule>();
	const auto Resolution = Rule->Resolve(Context);
	if (Resolution.Effect)
	{
		if (!Resolution.MagnitudeTag.IsValid() || !FMath::IsFinite(Resolution.Magnitude))
		{
			return {};
		}

		FGameplayEffectSpecHandle Spec;
		if (Context.bDetachedShot && Context.CapturedEffect.IsValid() &&
			Context.CapturedEffect.Data->Def->GetClass() == Resolution.Effect)
		{
			Spec = FGameplayEffectSpecHandle(new FGameplayEffectSpec(*Context.CapturedEffect.Data));
			auto EffectContext = Spec.Data->GetContext().Duplicate();
			EffectContext.AddHitResult(Context.Hit, true);
			Spec.Data->SetContext(EffectContext);
		}
		else if (Context.SourceASC)
		{
			auto EffectContext = Context.SourceASC->MakeEffectContext();
			EffectContext.AddHitResult(Context.Hit);
			Spec = Context.SourceASC->MakeOutgoingSpec(Resolution.Effect, 1.f, EffectContext);
		}

		if (!Spec.IsValid())
		{
			return {};
		}

		Spec.Data->SetSetByCallerMagnitude(Resolution.MagnitudeTag, Resolution.Magnitude);
		const auto Applied = Context.SourceASC ? Context.SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, Context.TargetASC) :
			Context.TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);

		// A rejected damage effect leaves no outcome: guard, stagger and feedback are not committed.
		if (!Applied.WasSuccessfullyApplied())
		{
			return {};
		}
	}

	Rule->Commit(Context, Resolution);
	for (UActorComponent* Component : Context.Target->GetComponents())
	{
		if (auto* Receiver = Cast<ICCLHitReceiver>(Component))
		{
			Receiver->ReceiveHit(Context, Resolution.Outcome);
		}
	}

	return Resolution.Outcome;
}
