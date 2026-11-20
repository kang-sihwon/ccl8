#include "CCLProgressionAssetLibrary.h"

#if WITH_EDITOR
#include "Agents/CCLAgentAIController.h"
#include "Agents/CCLLifeSimulation.h"
#include "Agents/CCLAgentTags.h"
#include "StateTree.h"
#include "StateTreeEditorData.h"
#include "StateTreeState.h"
#include "StateTreeCompiler.h"
#include "StateTreeCompilerLog.h"
#include "Components/StateTreeAIComponentSchema.h"
#include "KSIActionComponent.h"
#include "Actions/CCLWeaponAbility.h"
#include "Combat/CCLProjectile.h"
#include "Combat/CCLHitRule.h"
#include "AbilitySystem/CCLGameplayTags.h"
#include "Items/CCLItemDefinition.h"
#include "Items/CCLSkillDefinition.h"
#include "AbilitySystem/CCLEffects.h"
#include "AbilitySystem/CCLAbilitySet.h"
#include "Combat/CCLCombatDefinition.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Items/CCLAttachmentProfile.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

namespace
{
	template <class T>
	T* EditorAsset(const TCHAR* Folder, const TCHAR* Name)
	{
		const FString Path = FString(Folder) + TEXT("/") + Name;
		if (FPackageName::DoesPackageExist(Path))
		{
			return LoadObject<T>(nullptr, *(Path + TEXT(".") + Name));
		}

		UPackage* Package = CreatePackage(*Path);
		T* Object = NewObject<T>(Package, Name, RF_Public | RF_Standalone);
		FAssetRegistryModule::AssetCreated(Object);
		return Object;
	}

	template <class T>
	T* ProgressionAsset(const TCHAR* Name)
	{
		return EditorAsset<T>(TEXT("/Game/Progression"), Name);
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
		return UPackage::SavePackage(
			Object->GetOutermost(), Object,
			*FPackageName::LongPackageNameToFilename(Object->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()), Args);
	}

	// 생성기가 정하지 않는 값이 남지 않도록 정의 전체를 초기화한다.
	void DescribeProgression(UCCLItemDefinition* Item, const TCHAR* Label, int32 MaxStack, const TCHAR* Description = TEXT(""))
	{
		Item->ItemFragments.Reset();
		Item->ItemName = FText::FromString(Label);
		Item->Description = FText::FromString(Description);
		Item->IconTexture = nullptr;
		Item->MaxStackCount = MaxStack;
	}

	void AllowHands(FCCLEquipmentFragment& Equipment)
	{
		Equipment.DefaultSlot = CCLItemTags::Slot_RightHand;
		Equipment.AllowedSlots.AddTag(CCLItemTags::Slot_LeftHand);
		Equipment.AllowedSlots.AddTag(CCLItemTags::Slot_RightHand);
	}

	void AttachToHands(FCCLEquipmentFragment& Equipment, FGameplayTag LeftPoint, FGameplayTag RightPoint, const FTransform& Offset)
	{
		for (int32 Hand = 0; Hand < 2; ++Hand)
		{
			FCCLItemAttachment& Binding = Equipment.Attachments.AddDefaulted_GetRef();
			Binding.Slot = CCLEquipment::SlotAt(Hand);
			Binding.Point = Hand == 0 ? LeftPoint : RightPoint;
			Binding.Offset = Offset;
		}
	}

	bool BuildFood(const TCHAR* Folder, const TCHAR* Name, const TCHAR* Label, const TCHAR* Description, double ShelfLife, double MoldDelay,
				   double MoldGrowth)
	{
		auto* Item = EditorAsset<UCCLItemDefinition>(Folder, Name);
		if (!Item)
		{
			return false;
		}

		DescribeProgression(Item, Label, 20, Description);
		return Item->ConfigurePerishable(ShelfLife, MoldDelay, MoldGrowth) && SaveProgression(Item);
	}
} // namespace
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

	DescribeProgression(Gauntlets, TEXT("철 장갑 (공격력 +10)"), 1);
	FCCLMeleeWeaponFragment Fists;
	AllowHands(Fists);
	Fists.Effect = UCCLPersistentPowerEffect::StaticClass();
	Fists.Magnitude = 10.f;
	Fists.Actions.Combat = LoadObject<UCCLCombatDefinition>(nullptr, TEXT("/Game/Combat/DA_PlayerAttack.DA_PlayerAttack"));
	Fists.Actions.Abilities = LoadObject<UCCLAbilitySet>(nullptr, TEXT("/Game/Combat/DA_PlayerAbilities.DA_PlayerAbilities"));
	Gauntlets->ItemFragments.Add(FInstancedStruct::Make(Fists));
	if (!Fists.Actions.Combat || !Fists.Actions.Abilities)
	{
		return false;
	}

	DescribeProgression(Potion, TEXT("회복약 (체력 +50)"), 20);
	FCCLItemFragment_ConsumableData Use;
	Use.Effect = UCCLHealthChangeEffect::StaticClass();
	Use.Magnitude = 50.f;
	Potion->ItemFragments.Add(FInstancedStruct::Make(Use));
	Power->Label = FText::FromString(TEXT("공격 훈련 (공격력 +5)"));
	Power->PointCost = 1;
	Power->Effect = UCCLPersistentPowerEffect::StaticClass();
	Power->Magnitude = 5.f;
	Vitality->Label = FText::FromString(TEXT("체력 훈련 (체력 +25)"));
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
	if (!Base)
	{
		return false;
	}

	const TCHAR* Names[] = {TEXT("DA_RaiderStrike"), TEXT("DA_WardenHeavy"), TEXT("DA_WardenSweep")};
	for (int32 Index = 0; Index < 3; ++Index)
	{
		auto* Attack = ProgressionAsset<UCCLCombatDefinition>(Names[Index]);
		if (!Attack)
		{
			return false;
		}

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
		if (!SaveProgression(Attack))
		{
			return false;
		}
	}

	return true;
#else
	return false;
#endif
}

bool UCCLProgressionAssetLibrary::CreateEquipmentAssets()
{
#if WITH_EDITOR
	const TCHAR* Names[] = {TEXT("DA_TrainingSword"), TEXT("DA_TrainingShield"), TEXT("DA_TrainingStaff"),	  TEXT("DA_TrainingArmor"),
							TEXT("DA_TrainingBoots"), TEXT("DA_TrainingCloak"),	 TEXT("DA_TrainingNecklace"), TEXT("DA_TrainingRing")};
	const TCHAR* Labels[] = {TEXT("훈련용 검"), TEXT("훈련용 방패"), TEXT("양손 지팡이"),	 TEXT("훈련용 갑옷"),
							 TEXT("훈련용 장화"), TEXT("훈련용 망토"),  TEXT("훈련용 목걸이"), TEXT("훈련용 반지")};
	const FGameplayTag Slots[] = {CCLItemTags::Slot_RightHand, CCLItemTags::Slot_RightHand, CCLItemTags::Slot_RightHand,
								  CCLItemTags::Slot_Armor,	   CCLItemTags::Slot_Boots,		CCLItemTags::Slot_Cloak,
								  CCLItemTags::Slot_Necklace,  CCLItemTags::Slot_RingOne};
	UCCLCombatDefinition* Attack = LoadObject<UCCLCombatDefinition>(nullptr, TEXT("/Game/Combat/DA_PlayerAttack.DA_PlayerAttack"));
	UCCLAbilitySet* Abilities = LoadObject<UCCLAbilitySet>(nullptr, TEXT("/Game/Combat/DA_PlayerAbilities.DA_PlayerAbilities"));
	UStaticMesh* Meshes[] = {LoadObject<UStaticMesh>(nullptr, TEXT("/Game/TestResources/Weapon_Pack/Mesh/Weapons/Weapons_Kit/SM_Sword.SM_Sword")),
													 LoadObject<UStaticMesh>(nullptr, TEXT("/Game/TestResources/Weapon_Pack/Mesh/Weapons/Weapons_Kit/SM_Shield.SM_Shield")),
																			 LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"))};
	if (!Attack || !Abilities || !Meshes[0] || !Meshes[1] || !Meshes[2])
	{
		return false;
	}

	// 검과 방패의 메시와 오프셋은 시험 자원 교체(create_test_laboratories.py) 뒤 맞춘 값이다.
	const FTransform Offsets[] = {FTransform(FQuat(0., 0., 1., 0.), FVector::ZeroVector, FVector(0.75)),
								  FTransform(FQuat(0., -UE_INV_SQRT_2, 0., UE_INV_SQRT_2), FVector::ZeroVector, FVector(0.85)),
								  FTransform(FQuat::Identity, FVector(0., 0., 25.), FVector(0.06, 0.06, 1.5))};
	const ECCLHandAction LeftActions[] = {ECCLHandAction::Attack, ECCLHandAction::Guard, ECCLHandAction::Attack};
	const ECCLHandAction RightActions[] = {ECCLHandAction::Attack, ECCLHandAction::Guard, ECCLHandAction::Guard};
	for (int32 Index = 0; Index < 8; ++Index)
	{
		auto* Item = ProgressionAsset<UCCLItemDefinition>(Names[Index]);
		if (!Item)
		{
			return false;
		}

		DescribeProgression(Item, Labels[Index], 1, TEXT("장비 기능을 시험하기 위한 시제품이다. 외형과 수치는 추후 조정될 수 있다."));
		if (Index >= 3)
		{
			FCCLEquipmentFragment Equipment;
			Equipment.DefaultSlot = Slots[Index];
			Equipment.AllowedSlots.AddTag(Slots[Index]);
			if (Index == 7)
			{
				Equipment.AllowedSlots.AddTag(CCLItemTags::Slot_RingTwo);
			}

			Item->ItemFragments.Add(FInstancedStruct::Make(Equipment));
		}
		else
		{
			// 방패는 손 장비이고 검과 지팡이는 근접 무기다.
			FInstancedStruct Hand = Index == 1 ? FInstancedStruct::Make(FCCLHandEquipmentFragment()) : FInstancedStruct::Make(FCCLMeleeWeaponFragment());
			FCCLHandEquipmentFragment& Equipment = Hand.GetMutable<FCCLHandEquipmentFragment>();
			AllowHands(Equipment);
			Equipment.HandUsage = Index == 2 ? CCLItemTags::HandUsage_TwoHanded : CCLItemTags::HandUsage_OneHanded;
			Equipment.Actions.Combat = Attack;
			Equipment.Actions.Abilities = Abilities;
			Equipment.Actions.LeftAction = LeftActions[Index];
			Equipment.Actions.RightAction = RightActions[Index];
			AttachToHands(Equipment, Index == 1 ? CCLItemTags::Attachment_ShieldLeft : CCLItemTags::Attachment_GripLeft,
						  Index == 1 ? CCLItemTags::Attachment_ShieldRight : CCLItemTags::Attachment_GripRight, Offsets[Index]);
			Item->ItemFragments.Add(MoveTemp(Hand));
			FCCLVisualFragment Visual;
			Visual.WorldMesh = Meshes[Index];
			Item->ItemFragments.Add(FInstancedStruct::Make(Visual));
		}

		if (!SaveProgression(Item))
		{
			return false;
		}
	}

	return true;
#else
	return false;
#endif
}

bool UCCLProgressionAssetLibrary::CreateAttachmentProfileAssets()
{
#if WITH_EDITOR
	auto* Mesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (!Mesh)
	{
		return false;
	}

	const FName Names[] = {TEXT("CCL_Grip_L"), TEXT("CCL_Grip_R"), TEXT("CCL_Shield_L"), TEXT("CCL_Shield_R"), TEXT("CCL_Stow_Back")};
	const FName Bones[] = {TEXT("hand_l"), TEXT("hand_r"), TEXT("hand_l"), TEXT("hand_r"), TEXT("spine_03")};
	const FGameplayTag Points[] = {CCLItemTags::Attachment_GripLeft, CCLItemTags::Attachment_GripRight, CCLItemTags::Attachment_ShieldLeft,
								   CCLItemTags::Attachment_ShieldRight, CCLItemTags::Attachment_StowBack};
	for (FName Bone : Bones)
	{
		if (Mesh->GetRefSkeleton().FindBoneIndex(Bone) == INDEX_NONE)
		{
			return false;
		}
	}

	bool bChanged = false;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Names); ++Index)
	{
		if (Mesh->FindSocket(Names[Index]))
		{
			continue;
		}

		auto* Socket = NewObject<USkeletalMeshSocket>(Mesh, NAME_None, RF_Transactional);
		Socket->SocketName = Names[Index];
		Socket->BoneName = Bones[Index];
		Mesh->GetMeshOnlySocketList().Add(Socket);
		bChanged = true;
	}

	if (bChanged && !SaveProgression(Mesh))
	{
		return false;
	}

	if (FPackageName::DoesPackageExist(TEXT("/Game/Progression/DA_HumanoidAttachments")))
	{
		return true;
	}

	auto* Profile = ProgressionAsset<UCCLAttachmentProfile>(TEXT("DA_HumanoidAttachments"));
	if (!Profile)
	{
		return false;
	}

	Profile->ReferenceMesh = Mesh;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Names); ++Index)
	{
		FCCLAttachmentSocket Binding;
		Binding.Point = Points[Index];
		Binding.Socket = Names[Index];
		Profile->Bindings.Add(Binding);
	}

	Profile->PreviewItem = LoadObject<UCCLItemDefinition>(nullptr, TEXT("/Game/Progression/DA_TrainingSword.DA_TrainingSword"));
	TArray<FText> Errors;
	return Profile->ValidateProfile(Errors) && SaveProgression(Profile);
#else
	return false;
#endif
}

bool UCCLProgressionAssetLibrary::MigrateEquipmentTagAssets()
{
#if WITH_EDITOR
	if (!CreateAttachmentProfileAssets())
	{
		return false;
	}

	auto* Profile = LoadObject<UCCLAttachmentProfile>(nullptr, TEXT("/Game/Progression/DA_HumanoidAttachments.DA_HumanoidAttachments"));
	if (!Profile)
	{
		return false;
	}

	const TCHAR* Packages[] = {TEXT("/Game/Combat/DA_Unarmed.DA_Unarmed"),
							   TEXT("/Game/Combat/DA_EnemyUnarmed.DA_EnemyUnarmed"),
							   TEXT("/Game/Progression/DA_IronGauntlets.DA_IronGauntlets"),
							   TEXT("/Game/Progression/DA_RecoveryPotion.DA_RecoveryPotion"),
							   TEXT("/Game/Progression/DA_TrainingSword.DA_TrainingSword"),
							   TEXT("/Game/Progression/DA_TrainingShield.DA_TrainingShield"),
							   TEXT("/Game/Progression/DA_TrainingStaff.DA_TrainingStaff"),
							   TEXT("/Game/Progression/DA_TrainingArmor.DA_TrainingArmor"),
							   TEXT("/Game/Progression/DA_TrainingBoots.DA_TrainingBoots"),
							   TEXT("/Game/Progression/DA_TrainingCloak.DA_TrainingCloak"),
							   TEXT("/Game/Progression/DA_TrainingNecklace.DA_TrainingNecklace"),
							   TEXT("/Game/Progression/DA_TrainingRing.DA_TrainingRing")};
	TArray<UCCLItemDefinition*> Items;
	for (const TCHAR* Path : Packages)
	{
		auto* Item = LoadObject<UCCLItemDefinition>(nullptr, Path);
		if (!Item)
		{
			return false;
		}

		// Upgrade only the known prototype's old generic grip bindings; preserve custom points and offsets.
		if (Item->GetFName() == TEXT("DA_TrainingShield"))
		{
			for (auto& Fragment : Item->ItemFragments)
			{
				if (auto* Equipment = Fragment.GetMutablePtr<FCCLEquipmentFragment>())
				{
					for (auto& Binding : Equipment->Attachments)
					{
						if (Binding.Point == CCLItemTags::Attachment_GripLeft)
						{
							Binding.Point = CCLItemTags::Attachment_ShieldLeft;
						}

						if (Binding.Point == CCLItemTags::Attachment_GripRight)
						{
							Binding.Point = CCLItemTags::Attachment_ShieldRight;
						}
					}
				}
			}
		}

		FString Error;
		if (!Profile->ValidateItem(Item, Profile->ReferenceMesh, Error))
		{
			UE_LOG(LogTemp, Error, TEXT("CCL_TAG_MIGRATION invalid %s: %s"), Path, *Error);
			return false;
		}

		Items.Add(Item);
	}

	for (auto* Item : Items)
	{
		if (!SaveProgression(Item))
		{
			return false;
		}
	}

	return true;
#else
	return false;
#endif
}

bool UCCLProgressionAssetLibrary::CreateProjectileAssets()
{
#if WITH_EDITOR
	auto* Bullets = ProgressionAsset<UCCLItemDefinition>(TEXT("DA_Bullets"));
	DescribeProgression(Bullets, TEXT("탄환"), 100);
	if (!SaveProgression(Bullets))
	{
		return false;
	}

	auto* Actions = ProgressionAsset<UKSIActionSet>(TEXT("DA_FirearmActions"));
	Actions->Actions.Reset();
	FKSIActionGrant Fire;
	Fire.Action = CCLActionTags::Fire;
	Fire.Ability = UCCLProjectileAbility::StaticClass();
	Actions->Actions.Add(Fire);
	FKSIActionGrant Reload;
	Reload.Action = CCLActionTags::Reload;
	Reload.Ability = UCCLReloadAbility::StaticClass();
	Actions->Actions.Add(Reload);
	FKSIActionGrant Aim;
	Aim.Action = CCLActionTags::Aim;
	Aim.Ability = UCCLAimAbility::StaticClass();
	Actions->Actions.Add(Aim);
	if (!SaveProgression(Actions))
	{
		return false;
	}

	for (int32 Index = 0; Index < 2; ++Index)
	{
		const bool bRifle = Index == 1;
		auto* Combat = ProgressionAsset<UCCLCombatDefinition>(bRifle ? TEXT("DA_RifleHit") : TEXT("DA_PistolHit"));
		Combat->Damage = bRifle ? 40.f : 20.f;
		Combat->Cost = 0;
		Combat->DamageEffect = UCCLHealthChangeEffect::StaticClass();
		Combat->MagnitudeTag = CCLTags::Data_Magnitude;
		Combat->HitRule = UCCLDuelHitRule::StaticClass();
		Combat->bParryable = 0;
		auto* Profile = ProgressionAsset<UCCLProjectileProfile>(bRifle ? TEXT("DA_RifleBallistics") : TEXT("DA_PistolBallistics"));
		Profile->Combat = Combat;
		Profile->ProjectileClass = ACCLProjectile::StaticClass();
		Profile->Speed = bRifle ? 10000.f : 6000.f;
		Profile->MagazineSize = bRifle ? 1 : 6;
		Profile->ReloadSeconds = bRifle ? 4.f : 2.f;
		Profile->FireInterval = bRifle ? 1.f : 0.4f;
		Profile->Ammunition = Bullets;
		auto* Item = ProgressionAsset<UCCLItemDefinition>(bRifle ? TEXT("DA_Rifle") : TEXT("DA_Pistol"));
		DescribeProgression(Item, bRifle ? TEXT("저속 장전 소총") : TEXT("권총"), 1);
		FCCLRangedWeaponFragment Weapon;
		AllowHands(Weapon);
		Weapon.HandUsage = bRifle ? CCLItemTags::HandUsage_TwoHanded : CCLItemTags::HandUsage_OneHanded;
		Weapon.Profile = Profile;
		Weapon.Actions.Combat = Combat;
		Weapon.Actions.ActionSet = Actions;
		Weapon.Actions.HandActions.Add(CCLItemTags::Slot_LeftHand, CCLActionTags::Fire);
		Weapon.Actions.HandActions.Add(CCLItemTags::Slot_RightHand, bRifle ? CCLActionTags::Aim : CCLActionTags::Fire);
		AttachToHands(Weapon, CCLItemTags::Attachment_GripLeft, CCLItemTags::Attachment_GripRight, FTransform::Identity);
		Item->ItemFragments.Add(FInstancedStruct::Make(Weapon));
		FCCLVisualFragment Visual;
		Visual.WorldMesh = LoadObject<UStaticMesh>(nullptr, bRifle ? TEXT("/Game/Weapons/Rifle/Meshes/SM_Rifle.SM_Rifle") : TEXT("/Game/Weapons/Pistol/Meshes/SM_Pistol.SM_Pistol"));
		Item->ItemFragments.Add(FInstancedStruct::Make(Visual));
		if (!Visual.WorldMesh || !SaveProgression(Combat) || !SaveProgression(Profile) || !SaveProgression(Item))
		{
			return false;
		}
	}

	return true;
#else
	return false;
#endif
}

bool UCCLProgressionAssetLibrary::RebuildItemDefinitions()
{
#if WITH_EDITOR
	auto* Unarmed = EditorAsset<UCCLItemDefinition>(TEXT("/Game/Combat"), TEXT("DA_Unarmed"));
	auto* EnemyUnarmed = EditorAsset<UCCLItemDefinition>(TEXT("/Game/Combat"), TEXT("DA_EnemyUnarmed"));
	auto* PlayerAttack = LoadObject<UCCLCombatDefinition>(nullptr, TEXT("/Game/Combat/DA_PlayerAttack.DA_PlayerAttack"));
	auto* EnemyAttack = LoadObject<UCCLCombatDefinition>(nullptr, TEXT("/Game/Combat/DA_EnemyAttack.DA_EnemyAttack"));
	auto* PlayerAbilities = LoadObject<UCCLAbilitySet>(nullptr, TEXT("/Game/Combat/DA_PlayerAbilities.DA_PlayerAbilities"));
	auto* EnemyAbilities = LoadObject<UCCLAbilitySet>(nullptr, TEXT("/Game/Combat/DA_EnemyAbilities.DA_EnemyAbilities"));
	if (!Unarmed || !EnemyUnarmed || !PlayerAttack || !EnemyAttack || !PlayerAbilities || !EnemyAbilities)
	{
		return false;
	}

	ConfigureUnarmedItem(*Unarmed, PlayerAttack, PlayerAbilities, TEXT("맨손"));
	ConfigureUnarmedItem(*EnemyUnarmed, EnemyAttack, EnemyAbilities, TEXT("적의 맨손"));
	return SaveProgression(Unarmed) && SaveProgression(EnemyUnarmed) && CreateProgressionAssets() && CreateEquipmentAssets() &&
		   CreateProjectileAssets() &&
		   BuildFood(TEXT("/Game/Progression"), TEXT("DA_Bread"), TEXT("빵"), TEXT("보관 온도와 습도에 따라 신선도와 곰팡이가 변하는 음식."), 86400., 43200., 43200.) &&
																	   BuildFood(TEXT("/Game/Environment/Experiments"), TEXT("DA_FoodSample"), TEXT("실험용 빵"), TEXT("부패 기준 300초의 보관 조건 비교 표본."), 300., 120., 180.);
#else
	return false;
#endif
}

void UCCLProgressionAssetLibrary::ConfigureUnarmedItem(UCCLItemDefinition& Item, UCCLCombatDefinition* Combat, UCCLAbilitySet* Abilities, const TCHAR* Label)
{
#if WITH_EDITOR
	// 맨손은 단계 3에서 무브셋으로 바뀐다. 그때까지는 양손에 들 수 있는 근접 무기로 둔다.
	DescribeProgression(&Item, Label, 1);
	FCCLMeleeWeaponFragment Fists;
	AllowHands(Fists);
	Fists.Actions.Combat = Combat;
	Fists.Actions.Abilities = Abilities;
	Item.ItemFragments.Add(FInstancedStruct::Make(Fists));
#endif
}

bool UCCLProgressionAssetLibrary::CreateAgentAssets()
{
#if WITH_EDITOR
	auto* Scenario = ProgressionAsset<UCCLPopulationScenario>(TEXT("DA_MerchantLifeScenario"));
	Scenario->InitialState = FCCLLifeSimulation::MerchantScenario(42);
	auto* Tree = ProgressionAsset<UStateTree>(TEXT("ST_LifeAgent"));
	auto* Data = NewObject<UStateTreeEditorData>(Tree);
	Tree->EditorData = Data;
	Data->Schema = NewObject<UStateTreeAIComponentSchema>(Data);
	auto& Root = Data->AddRootState();
	auto& Select = Root.AddChildState(TEXT("Select intent"));
	auto& Approach = Root.AddChildState(TEXT("Approach opportunity"));
	auto& Perform = Root.AddChildState(TEXT("Perform selected activity"));
	Select.AddTask<FCCLAgentTask>(ECCLAgentExecutionPhase::Select);
	Approach.AddTask<FCCLAgentTask>(ECCLAgentExecutionPhase::Approach);
	Perform.AddTask<FCCLAgentTask>(ECCLAgentExecutionPhase::Perform);
	Select.AddTransition(EStateTreeTransitionTrigger::OnStateSucceeded, EStateTreeTransitionType::GotoState, &Approach);
	Approach.AddTransition(EStateTreeTransitionTrigger::OnStateSucceeded, EStateTreeTransitionType::GotoState, &Perform);
	Approach.AddTransition(EStateTreeTransitionTrigger::OnStateFailed, EStateTreeTransitionType::GotoState, &Select);
	Perform.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::GotoState, &Select);
	FStateTreeCompilerLog Log;
	FStateTreeCompiler Compiler(Log);
	return Compiler.Compile(*Tree) && SaveProgression(Tree) && SaveProgression(Scenario);
#else
	return false;
#endif
}

bool UCCLProgressionAssetLibrary::MigrateAgentBiologyAssets()
{
#if WITH_EDITOR
	auto* Scenario = LoadObject<UCCLPopulationScenario>(nullptr,
		TEXT("/Game/Progression/DA_MerchantLifeScenario.DA_MerchantLifeScenario"));
	if (!Scenario)
	{
		return false;
	}

	auto Candidate = Scenario->InitialState;
	const auto Defaults = FCCLLifeSimulation::MerchantScenario(Candidate.Seed);
	int32 Added = 0;
	for (auto& Agent : Candidate.Agents)
	{
		if (Agent.Features.Contains(CCLAgentTags::Feature_Biology))
		{
			continue;
		}

		const auto* Default = Defaults.Agents.FindByPredicate([&](const FKSIAgentRecord& R)
		{
			return R.Id == Agent.Id && R.DefinitionId == Agent.DefinitionId;
		});
		if (Default)
		{
			Agent.Features.Add(CCLAgentTags::Feature_Biology, Default->Features[CCLAgentTags::Feature_Biology]);
			++Added;
		}
	}

	FCCLLifeSimulation Validation;
	FString Error;
	if (!Validation.Initialize(Candidate, Error))
	{
		UE_LOG(LogTemp, Error, TEXT("CCL_AGENT_BIOLOGY_ASSET invalid candidate: %s"), *Error);
		return false;
	}

	if (Added > 0)
	{
		const auto Previous = Scenario->InitialState;
		Scenario->InitialState = MoveTemp(Candidate);
		if (!SaveProgression(Scenario))
		{
			Scenario->InitialState = Previous;
			return false;
		}
	}

	UE_LOG(LogTemp, Display, TEXT("CCL_AGENT_BIOLOGY_ASSET PASS added=%d residents=%d"), Added, Scenario->InitialState.Agents.Num());
	return true;
#else
	return false;
#endif
}
