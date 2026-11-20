#include "KSIItemDefinition.h"

#include "Misc/DataValidation.h"

#if WITH_EDITOR
EDataValidationResult UKSIItemDefinition::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult Parent = Super::IsDataValid(Context);
	TArray<FText> Errors;
	ValidateDefinition(Errors);
	for (const FText& Error : Errors)
	{
		Context.AddError(Error);
	}
	return Errors.IsEmpty() && Parent != EDataValidationResult::Invalid ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}
#endif

bool UKSIItemDefinition::ValidateDefinition(TArray<FText>& Errors) const
{
	if (MaxStackCount < 1 || MaxStackCount > 1000)
	{
		Errors.Add(FText::FromString(TEXT("MaxStackCount must be between 1 and 1000.")));
	}

	const FKSIItemValidation Context{*this, Errors};
	for (const FInstancedStruct& Fragment : ItemFragments)
	{
		const FKSIItemFragmentData* Value = Fragment.GetPtr<FKSIItemFragmentData>();
		if (!Value)
		{
			Errors.Add(FText::FromString(TEXT("Invalid item fragment type.")));
			continue;
		}

		Value->Validate(Context);
	}
	return Errors.IsEmpty();
}

FText UKSIItemDefinition::GetLabel() const
{
	return ItemName.IsEmpty() ? FText::FromString(GetName()) : ItemName;
}

int32 UKSIItemDefinition::GetMaxStack() const
{
	return FMath::Clamp(MaxStackCount, 1, 1000);
}
