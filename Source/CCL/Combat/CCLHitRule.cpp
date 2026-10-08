#include "CCLHitRule.h"

#include "Agents/CCLAgentComponent.h"
#include "AbilitySystem/CCLOffenseSet.h"

#include "CCLCombatDefinition.h"
#include "CCLFighterComponent.h"
#include "AbilitySystem/CCLAbilitySystemComponent.h"
#include "AbilitySystem/CCLGameplayTags.h"
#include "AbilitySystem/CCLEffects.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "AbilitySystem/CCLStaminaSet.h"

FCCLHitResolution UCCLHitRule::Resolve(const FCCLHitContext& Context) const
{
	return {};
}

FCCLHitResolution UCCLDuelHitRule::Resolve(const FCCLHitContext& Context) const
{
	if ((!Context.bDetachedShot && Context.SourceASC->HasMatchingGameplayTag(CCLTags::State_Dead)) || Context.TargetASC->HasMatchingGameplayTag(CCLTags::State_Dead))
	{
		return FCCLHitResolution(FGameplayTag(CCLTags::Outcome_Rejected));
	}

	if (!Context.TargetASC->HasAttributeSetForAttribute(UCCLHealthSet::GetHealthAttribute()) || Context.TargetASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) <= 0.f)
	{
		return FCCLHitResolution(FGameplayTag(CCLTags::Outcome_Rejected));
	}

	const auto* SourceFighter = Context.Source->FindComponentByClass<UCCLFighterComponent>();
	auto* TargetFighter = Context.Target->FindComponentByClass<UCCLFighterComponent>();

	const int32 SourceTeam = Context.bDetachedShot ? Context.CapturedTeam : (SourceFighter ? SourceFighter->Team : INDEX_NONE);
	if (SourceTeam != INDEX_NONE && TargetFighter && SourceTeam == TargetFighter->Team)
	{
		return FCCLHitResolution(FGameplayTag(CCLTags::Outcome_Rejected));
	}

	if (Context.TargetASC->HasMatchingGameplayTag(CCLTags::State_Invulnerable))
	{
		return FCCLHitResolution(FGameplayTag(CCLTags::Outcome_Evaded));
	}

	if (!TargetFighter)
	{
		const float Bonus = !Context.bDetachedShot && Context.SourceASC->GetSet<UCCLOffenseSet>() ? Context.SourceASC->GetNumericAttribute(UCCLOffenseSet::GetAttackBonusAttribute()) : 0.f;
		return {CCLTags::Outcome_Damage, Context.Definition->DamageEffect, Context.Definition->MagnitudeTag, -(Context.Definition->Damage + Bonus)};
	}

	auto* TargetASC = Cast<UCCLAbilitySystemComponent>(Context.TargetASC);

	if (!TargetASC)
	{
		return FCCLHitResolution(FGameplayTag(CCLTags::Outcome_Rejected));
	}

	const FVector ToAttacker = Context.bDetachedShot ? -Context.IncomingDirection.GetSafeNormal2D() :
		(Context.Source->GetActorLocation() - Context.Target->GetActorLocation()).GetSafeNormal2D();
	const bool bFrontal = FVector::DotProduct(Context.Target->GetActorForwardVector(), ToAttacker) >= FMath::Cos(FMath::DegreesToRadians(TargetFighter->DefenseAngle * 0.5f));

	if (bFrontal && Context.Definition->bParryable && TargetASC->HasMatchingGameplayTag(CCLTags::State_Parry))
	{
		if (!Context.bDetachedShot)
		{
			Context.SourceASC->CancelAllAbilities();
			if (auto* SourceASC = Cast<UCCLAbilitySystemComponent>(Context.SourceASC))
			{
				SourceASC->ApplyEffect(UCCLStaggerEffect::StaticClass(), 0.f, TargetFighter->ParryStaggerDuration);
			}
		}

		TargetFighter->NotifyHit(CCLTags::Outcome_Parried);
		return FCCLHitResolution(FGameplayTag(CCLTags::Outcome_Parried));
	}

	if (bFrontal && Context.Definition->bGuardable && TargetASC->HasMatchingGameplayTag(CCLTags::State_Guard) && TargetASC->GetSet<UCCLStaminaSet>())
	{
		const float Stamina = TargetASC->GetNumericAttribute(UCCLStaminaSet::GetStaminaAttribute());
		TargetASC->ApplyEffect(UCCLStaminaChangeEffect::StaticClass(), -TargetFighter->GuardCost);
		const bool bBroken = Stamina <= TargetFighter->GuardCost;

		if (bBroken)
		{
			TargetASC->CancelAllAbilities();
			TargetASC->ApplyEffect(UCCLStaggerEffect::StaticClass(), 0.f, TargetFighter->GuardBreakDuration);
		}

		const FGameplayTag Result = bBroken ? CCLTags::Outcome_GuardBroken : CCLTags::Outcome_Guarded;
		TargetFighter->NotifyHit(Result);
		return Result;
	}

	TargetASC->CancelAllAbilities();
	TargetFighter->NotifyHit(CCLTags::Outcome_Damage);
	const float Bonus = !Context.bDetachedShot && Context.SourceASC->GetSet<UCCLOffenseSet>() ? Context.SourceASC->GetNumericAttribute(UCCLOffenseSet::GetAttackBonusAttribute()) : 0.f;
	return {CCLTags::Outcome_Damage, Context.Definition->DamageEffect, Context.Definition->MagnitudeTag, -(Context.Definition->Damage + Bonus)};
}

FGameplayTag CCLHit::Apply(const FCCLHitContext& Context)
{
	if (!IsValid(Context.Source) || !IsValid(Context.Target) || !Context.Source->HasAuthority() ||
		Context.Source == Context.Target || (!Context.SourceASC && !(Context.bDetachedShot && Context.CapturedEffect.IsValid())) || !Context.TargetASC ||
		Context.TargetASC->GetAvatarActor() != Context.Target || !Context.Definition || !Context.Definition->HitRule)
	{
		return {};
	}

	const auto Resolution = Context.Definition->HitRule->GetDefaultObject<UCCLHitRule>()->Resolve(Context);
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
		if (!Applied.WasSuccessfullyApplied())
		{
			return {};
		}
	}

	if (Resolution.Outcome == CCLTags::Outcome_Damage)
	{
		if (auto* Agent = Context.Target->FindComponentByClass<UCCLAgentComponent>())
		{
			const float Health = Context.TargetASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute());
			const float Maximum = Context.TargetASC->GetNumericAttribute(UCCLHealthSet::GetMaxHealthAttribute());
			Agent->RecordDamage(Context.Source, Health / FMath::Max(1.f, Maximum));
		}
	}
	return Resolution.Outcome;
}
