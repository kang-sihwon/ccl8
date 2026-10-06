#include "CCLProgressionAssetLibrary.h"

#if WITH_EDITOR
#include "Items/CCLItemDefinition.h"
#include "Items/CCLSkillDefinition.h"
#include "AbilitySystem/CCLEffects.h"
#include "AbilitySystem/CCLAbilitySet.h"
#include "Combat/CCLCombatDefinition.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

namespace
{
	template <class T>
	T* Asset(const TCHAR* Name)
	{
		const FString Path = FString(TEXT("/Game/Progression/")) + Name;
		if (FPackageName::DoesPackageExist(Path))
		{
			return LoadObject<T>(nullptr, *(Path + TEXT(".") + Name));
		}
		UPackage* Package = CreatePackage(*Path);
		T* Object = NewObject<T>(Package, Name, RF_Public | RF_Standalone);
		FAssetRegistryModule::AssetCreated(Object);
		return Object;
	}

	bool Save(UObject* Object)
	{
		if (!Object)
		{
			return false;
		}
		Object->MarkPackageDirty();
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		return UPackage::SavePackage(Object->GetOutermost(), Object,
			*FPackageName::LongPackageNameToFilename(Object->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()), Args);
	}

	void Describe(UCCLItemDefinition* Item, const TCHAR* Label, int32 MaxStack)
	{
		Item->Fragments.Reset();
		auto* Display = NewObject<UCCLItemFragment_Display>(Item);
		Display->Label = FText::FromString(Label);
		Item->Fragments.Add(Display);
		auto* Stack = NewObject<UCCLItemFragment_Stack>(Item);
		Stack->MaxCount = MaxStack;
		Item->Fragments.Add(Stack);
	}
}
#endif

bool UCCLProgressionAssetLibrary::CreateProgressionAssets()
{
#if WITH_EDITOR
	auto* Gauntlets = Asset<UCCLItemDefinition>(TEXT("DA_IronGauntlets"));
	auto* Potion = Asset<UCCLItemDefinition>(TEXT("DA_RecoveryPotion"));
	auto* Power = Asset<UCCLSkillDefinition>(TEXT("DA_PowerTraining"));
	auto* Vitality = Asset<UCCLSkillDefinition>(TEXT("DA_VitalityTraining"));
	if (!Gauntlets || !Potion || !Power || !Vitality)
	{
		return false;
	}
	Describe(Gauntlets, TEXT("Iron Gauntlets (+10 attack)"), 1);
	auto* Equipment = NewObject<UCCLItemFragment_Equipment>(Gauntlets);
	Equipment->Effect = UCCLPersistentPowerEffect::StaticClass();
	Equipment->Magnitude = 10.f;
	Gauntlets->Fragments.Add(Equipment);
	auto* Combat = NewObject<UCCLItemFragment_Combat>(Gauntlets);
	Combat->Combat = LoadObject<UCCLCombatDefinition>(nullptr, TEXT("/Game/Combat/DA_PlayerAttack.DA_PlayerAttack"));
	Combat->Abilities = LoadObject<UCCLAbilitySet>(nullptr, TEXT("/Game/Combat/DA_PlayerAbilities.DA_PlayerAbilities"));
	Gauntlets->Fragments.Add(Combat);
	if (!Combat->Combat || !Combat->Abilities)
	{
		return false;
	}
	Describe(Potion, TEXT("Recovery Potion (+50 HP)"), 20);
	auto* Use = NewObject<UCCLItemFragment_Consumable>(Potion);
	Use->Effect = UCCLHealthChangeEffect::StaticClass();
	Use->Magnitude = 50.f;
	Potion->Fragments.Add(Use);
	Power->Label = FText::FromString(TEXT("Power Training (+5 attack)"));
	Power->PointCost = 1;
	Power->Effect = UCCLPersistentPowerEffect::StaticClass();
	Power->Magnitude = 5.f;
	Vitality->Label = FText::FromString(TEXT("Vitality Training (+25 HP)"));
	Vitality->PointCost = 1;
	Vitality->Effect = UCCLPersistentVitalityEffect::StaticClass();
	Vitality->Magnitude = 25.f;
	return Save(Gauntlets) && Save(Potion) && Save(Power) && Save(Vitality);
#else
	return false;
#endif
}
