#include "KSIHitRule.h"

#include "KSICombatDefinition.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"

FKSIHitResolution UKSIHitRule::Resolve(const FKSIHitContext& Context) const
{
    return {};
}

void UKSIHitRule::Commit(const FKSIHitContext& Context, const FKSIHitResolution& Resolution) const
{
}

FGameplayTag KSIHit::Apply(const FKSIHitContext& Context)
{
	if (!IsValid(Context.Source) || !IsValid(Context.Target) || !Context.Source->HasAuthority() ||
		Context.Source == Context.Target || Context.Source->GetWorld() != Context.Target->GetWorld() ||
		(!Context.bDetachedSource && (!IsValid(Context.SourceASC) || Context.SourceASC->GetAvatarActor() != Context.Source)) || (!Context.SourceASC && !(Context.bDetachedSource && Context.CapturedEffect.IsValid())) || !IsValid(Context.TargetASC) ||
		Context.TargetASC->GetAvatarActor() != Context.Target || !IsValid(Context.Definition) || !Context.Definition->HitRule)
	{
		return {};
	}

	const auto* Rule = Context.Definition->HitRule->GetDefaultObject<UKSIHitRule>();
	const auto Resolution = Rule->Resolve(Context);
	if (!Resolution.Outcome.IsValid())
	{
		return {};
	}
	if (Resolution.Effect)
	{
		if (!Resolution.MagnitudeTag.IsValid() || !FMath::IsFinite(Resolution.Magnitude))
		{
			return {};
		}

		FGameplayEffectSpecHandle Spec;
		if (Context.bDetachedSource && Context.CapturedEffect.IsValid() &&
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
	TInlineComponentArray<UActorComponent*> Receivers(Context.Target);
	for (UActorComponent* Component : Receivers)
	{
		if (auto* Receiver = IsValid(Component) ? Cast<IKSIHitReceiver>(Component) : nullptr)
		{
			Receiver->ReceiveHit(Context, Resolution.Outcome);
		}
	}

	return Resolution.Outcome;
}
