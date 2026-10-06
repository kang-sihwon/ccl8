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
	T* ProgressionAsset(const TCHAR* Name)
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

	bool SaveProgression(UObject* Object)
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

	void DescribeProgression(UCCLItemDefinition* Item, const TCHAR* Label, int32 MaxStack)
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
	auto* Gauntlets = ProgressionAsset<UCCLItemDefinition>(TEXT("DA_IronGauntlets"));
	auto* Potion = ProgressionAsset<UCCLItemDefinition>(TEXT("DA_RecoveryPotion"));
	auto* Power = ProgressionAsset<UCCLSkillDefinition>(TEXT("DA_PowerTraining"));
	auto* Vitality = ProgressionAsset<UCCLSkillDefinition>(TEXT("DA_VitalityTraining"));
	if (!Gauntlets || !Potion || !Power || !Vitality)
	{
		return false;
	}
	DescribeProgression(Gauntlets, TEXT("Iron Gauntlets (+10 attack)"), 1);
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
	DescribeProgression(Potion, TEXT("Recovery Potion (+50 HP)"), 20);
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
	return SaveProgression(Gauntlets) && SaveProgression(Potion) && SaveProgression(Power) && SaveProgression(Vitality);
#else
	return false;
#endif
}

bool UCCLProgressionAssetLibrary::CreateEncounterAssets()
{
#if WITH_EDITOR
	const auto* Base = LoadObject<UCCLCombatDefinition>(nullptr, TEXT("/Game/Combat/DA_EnemyAttack.DA_EnemyAttack"));
	if (!Base) { return false; }
	const TCHAR* Names[] = { TEXT("DA_RaiderStrike"), TEXT("DA_WardenHeavy"), TEXT("DA_WardenSweep") };
	for (int32 Index = 0; Index < 3; ++Index)
	{
		auto* Attack = ProgressionAsset<UCCLCombatDefinition>(Names[Index]);
		if (!Attack) { return false; }
		Attack->DamageEffect = Base->DamageEffect;
		Attack->MagnitudeTag = Base->MagnitudeTag;
		Attack->HitRule = Base->HitRule;
		Attack->Montage = Base->Montage;
		Attack->Damage = Index == 0 ? 12.f : (Index == 1 ? 30.f : 24.f);
		Attack->Cost = 0.f;
		Attack->Windup = Index == 0 ? 0.3f : (Index == 1 ? 1.05f : 0.75f);
		Attack->Active = 0.2f;
		Attack->Recovery = Index == 0 ? 0.45f : 0.7f;
		Attack->Reach = Index == 2 ? 230.f : 165.f;
		Attack->Radius = Index == 2 ? 100.f : 35.f;
		Attack->bGuardable = Index != 2;
		Attack->bParryable = Index != 2;
		if (!SaveProgression(Attack)) { return false; }
	}
	return true;
#else
	return false;
#endif
}
