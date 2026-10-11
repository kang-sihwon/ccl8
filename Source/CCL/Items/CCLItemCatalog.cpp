#include "CCLItemCatalog.h"

#include "CCLItemDefinition.h"
#include "CCLSkillDefinition.h"
#include "Misc/DataValidation.h"

namespace
{
	template <typename T>
	T* FindUnique(const TArray<TObjectPtr<T>>& Definitions, FName Name)
	{
		T* Found = nullptr;
		for (T* Definition : Definitions)
		{
			if (Definition && Definition->GetFName() == Name)
			{
				if (Found)
				{
					return nullptr;
				}
				Found = Definition;
			}
		}
		return Found;
	}

	template <typename T>
	void ValidateNames(const TArray<TObjectPtr<T>>& Definitions, TArray<FText>& Errors)
	{
		TSet<FName> Names;
		for (T* Definition : Definitions)
		{
			if (!Definition)
			{
				Errors.Add(FText::FromString(TEXT("Catalog contains an empty definition.")));
			}
			else if (Names.Contains(Definition->GetFName()))
			{
				Errors.Add(FText::FromString(FString::Printf(TEXT("Duplicate catalog save name: %s"), *Definition->GetName())));
			}
			else
			{
				Names.Add(Definition->GetFName());
			}
		}
	}
}

bool UCCLItemCatalog::ValidateCatalog(TArray<FText>& Errors) const
{
	const int32 Before = Errors.Num();
	ValidateNames(Items, Errors);
	ValidateNames(TrainingSkills, Errors);
	return Errors.Num() == Before;
}

UKSIItemDefinition* UCCLItemCatalog::FindItem(FName Name) const
{
	return FindUnique(Items, Name);
}

UCCLSkillDefinition* UCCLItemCatalog::FindSkill(FName Name) const
{
	return FindUnique(TrainingSkills, Name);
}

const UCCLItemCatalog* UCCLItemCatalog::Get()
{
	const auto* Catalog = GetDefault<UCCLItemCatalogSettings>()->Catalog.LoadSynchronous();
	TArray<FText> Errors;
	return Catalog && Catalog->ValidateCatalog(Errors) ? Catalog : nullptr;
}

#if WITH_EDITOR
EDataValidationResult UCCLItemCatalog::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult Parent = Super::IsDataValid(Context);
	TArray<FText> Errors;
	if (!ValidateCatalog(Errors))
	{
		for (const FText& Error : Errors)
		{
			Context.AddError(Error);
		}
		return EDataValidationResult::Invalid;
	}
	return Parent == EDataValidationResult::Invalid ? Parent : EDataValidationResult::Valid;
}
#endif
