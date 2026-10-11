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
	FKSIHitResolution Reject()
	{
		return FKSIHitResolution(FGameplayTag(CCLTags::Outcome_Rejected));
	}

	FKSIHitResolution Damage(const FKSIHitContext& Context)
	{
		const auto* Definition = CastChecked<UCCLCombatDefinition>(Context.Definition);
		const bool bBonus = !Context.bDetachedSource && Context.SourceASC && Context.SourceASC->GetSet<UCCLOffenseSet>();
		const float Bonus = bBonus ? Context.SourceASC->GetNumericAttribute(UCCLOffenseSet::GetAttackBonusAttribute()) : 0.f;
		return {CCLTags::Outcome_Damage, Definition->DamageEffect, Definition->MagnitudeTag, -(Definition->Damage + Bonus)};
	}
}

// 부모 인터페이스 함수

FKSIHitResolution UCCLDuelHitRule::Resolve(const FKSIHitContext& Context) const
{
	const auto* Definition = Cast<UCCLCombatDefinition>(Context.Definition);
	if (!Definition)
	{
		return {};
	}

	if ((!Context.bDetachedSource && Context.SourceASC->HasMatchingGameplayTag(CCLTags::State_Dead)) || Context.TargetASC->HasMatchingGameplayTag(CCLTags::State_Dead))
	{
		return Reject();
	}

	if (!Context.TargetASC->HasAttributeSetForAttribute(UCCLHealthSet::GetHealthAttribute()) || Context.TargetASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) <= 0.f)
	{
		return Reject();
	}

	const auto* SourceFighter = Context.Source->FindComponentByClass<UCCLFighterComponent>();
	const auto* TargetFighter = Context.Target->FindComponentByClass<UCCLFighterComponent>();
	const auto* Facts = Context.SourceFacts.GetPtr<FCCLHitSourceFacts>();
	const int32 SourceTeam = Context.bDetachedSource ? (Facts ? Facts->Team : INDEX_NONE) : (SourceFighter ? SourceFighter->Team : INDEX_NONE);
	if (SourceTeam != INDEX_NONE && TargetFighter && SourceTeam == TargetFighter->Team)
	{
		return Reject();
	}

	if (Context.TargetASC->HasMatchingGameplayTag(CCLTags::State_Invulnerable))
	{
		return FKSIHitResolution(FGameplayTag(CCLTags::Outcome_Evaded));
	}

	if (!TargetFighter)
	{
		return Damage(Context);
	}

	if (!Cast<UCCLAbilitySystemComponent>(Context.TargetASC))
	{
		return Reject();
	}

	const FVector ToAttacker = Context.bDetachedSource ? -Context.IncomingDirection.GetSafeNormal2D() :
		(Context.Source->GetActorLocation() - Context.Target->GetActorLocation()).GetSafeNormal2D();
	const bool bFrontal = FVector::DotProduct(Context.Target->GetActorForwardVector(), ToAttacker) >= FMath::Cos(FMath::DegreesToRadians(TargetFighter->DefenseAngle * 0.5f));

	if (bFrontal && Definition->bParryable && Context.TargetASC->HasMatchingGameplayTag(CCLTags::State_Parry))
	{
		return FKSIHitResolution(FGameplayTag(CCLTags::Outcome_Parried));
	}

	if (bFrontal && Definition->bGuardable && Context.TargetASC->HasMatchingGameplayTag(CCLTags::State_Guard) && Context.TargetASC->GetSet<UCCLStaminaSet>())
	{
		const float Stamina = Context.TargetASC->GetNumericAttribute(UCCLStaminaSet::GetStaminaAttribute());
		return FKSIHitResolution(FGameplayTag(Stamina <= TargetFighter->GuardCost ? CCLTags::Outcome_GuardBroken : CCLTags::Outcome_Guarded));
	}

	return Damage(Context);
}

void UCCLDuelHitRule::Commit(const FKSIHitContext& Context, const FKSIHitResolution& Resolution) const
{
	auto* TargetFighter = Context.Target->FindComponentByClass<UCCLFighterComponent>();
	auto* TargetASC = Cast<UCCLAbilitySystemComponent>(Context.TargetASC);
	if (!TargetFighter || !TargetASC)
	{
		return;
	}

	if (Resolution.Outcome == CCLTags::Outcome_Parried)
	{
		if (!Context.bDetachedSource && Context.SourceASC)
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
