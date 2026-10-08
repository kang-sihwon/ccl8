#include "CCLActionComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"

UCCLActionComponent::UCCLActionComponent()
{
	SetIsReplicatedByDefault(true);
}

void UCCLActionComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	while (!Sources.IsEmpty())
	{
		RemoveSource(Sources.Last().Id);
	}

	Super::EndPlay(Reason);
}

bool UCCLActionComponent::RegisterSource(FGuid Id, UObject* Definition, const TArray<FCCLActionGrant>& Grants)
{
	auto* ASC = GetASC();
	if (!GetOwner()->HasAuthority() || !ASC || !Id.IsValid() || !Definition || Grants.IsEmpty())
	{
		return false;
	}

	TSet<FGameplayTag> Tags;
	for (const auto& Grant : Grants)
	{
		if (!Grant.Action.IsValid() || !Grant.Ability || Tags.Contains(Grant.Action))
		{
			return false;
		}

		Tags.Add(Grant.Action);
	}

	if (Sources.ContainsByPredicate([Id](const FCCLActionSource& Source) { return Source.Id == Id; }))
	{
		return false;
	}

	FCCLActionSource Source;
	Source.Id = Id;
	Source.Definition = Definition;
	for (const auto& Grant : Grants)
	{
		FGameplayAbilitySpec Spec(Grant.Ability, 1, INDEX_NONE, Definition);
		Source.Handles.Add(Grant.Action, ASC->GiveAbility(Spec));
	}

	Sources.Add(MoveTemp(Source));
	return true;
}

void UCCLActionComponent::RemoveSource(FGuid Id)
{
	const int32 Index = Sources.IndexOfByPredicate([Id](const FCCLActionSource& Source) { return Source.Id == Id; });
	if (Index == INDEX_NONE)
	{
		return;
	}

	const auto Handles = Sources[Index].Handles;
	Sources.RemoveAt(Index);
	if (auto* ASC = GetASC(); ASC && GetOwner()->HasAuthority())
	{
		for (const auto& Pair : Handles)
		{
			ASC->CancelAbilityHandle(Pair.Value);
			ASC->ClearAbility(Pair.Value);
		}
	}
}

bool UCCLActionComponent::Execute(FGuid SourceId, FGameplayTag Action)
{
	auto* ASC = GetASC();
	const auto* Source = Sources.FindByPredicate([SourceId](const FCCLActionSource& S) { return S.Id == SourceId; });
	const auto* Handle = Source ? Source->Handles.Find(Action) : nullptr;
	return GetOwner()->HasAuthority() && ASC && Handle && ASC->TryActivateAbility(*Handle);
}

void UCCLActionComponent::Cancel(FGuid SourceId)
{
	auto* ASC = GetASC();
	const auto* Source = Sources.FindByPredicate([SourceId](const FCCLActionSource& S) { return S.Id == SourceId; });
	if (ASC && Source && GetOwner()->HasAuthority())
	{
		const auto Handles = Source->Handles;
		for (const auto& Pair : Handles)
		{
			ASC->CancelAbilityHandle(Pair.Value);
		}
	}
}

const FCCLActionSource* UCCLActionComponent::FindSource(FGameplayAbilitySpecHandle Ability) const
{
	return Sources.FindByPredicate([Ability](const FCCLActionSource& S)
	{
		for (const auto& Pair : S.Handles)
		{
			if (Pair.Value == Ability)
			{
				return true;
			}
		}

		return false;
	});
}

void UCCLActionComponent::Release(FGuid SourceId, FGameplayTag Action)
{
	const auto* Source = Sources.FindByPredicate([SourceId](const FCCLActionSource& S) { return S.Id == SourceId; });
	const auto* Handle = Source ? Source->Handles.Find(Action) : nullptr;
	if (auto* ASC = GetASC(); GetOwner()->HasAuthority() && ASC && Handle)
	{
		ASC->CancelAbilityHandle(*Handle);
	}
}

bool UCCLActionComponent::HasSource(FGuid Id) const
{
	return Sources.ContainsByPredicate([Id](const FCCLActionSource& S) { return S.Id == Id; });
}

UAbilitySystemComponent* UCCLActionComponent::GetASC() const
{
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
}
