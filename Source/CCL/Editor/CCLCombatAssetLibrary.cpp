#include "CCLCombatAssetLibrary.h"

#if WITH_EDITOR
#include "AbilitySystem/CCLAbilitySet.h"
#include "AbilitySystem/CCLCombatAbility.h"
#include "AbilitySystem/CCLGameplayTags.h"
#include "Combat/CCLCombatDefinition.h"
#include "Combat/CCLHitRule.h"
#include "AbilitySystem/CCLEffects.h"
#include "Combat/CCLEnemyAIController.h"
#include "Items/CCLItemDefinition.h"
#include "Editor.h"
#include "ActorFactories/ActorFactory.h"
#include "EngineUtils.h"
#include "WorldPartition/WorldPartition.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "Builders/CubeBuilder.h"
#include "NavigationSystem.h"
#include "StateTree.h"
#include "StateTreeEditorData.h"
#include "StateTreeState.h"
#include "StateTreeCompiler.h"
#include "StateTreeCompilerLog.h"
#include "Components/StateTreeAIComponentSchema.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

namespace
{
	template <class T>
	T* Asset(const TCHAR* Name)
	{
		const FString Path = FString(TEXT("/Game/Combat/")) + Name;

		if (T* Existing = LoadObject<T>(nullptr, *(Path + TEXT(".") + Name)))
		{
			return Existing;
		}

		UPackage* Package = CreatePackage(*Path);
		T* Object = NewObject<T>(Package, Name, RF_Public | RF_Standalone);
		FAssetRegistryModule::AssetCreated(Object);
		return Object;
	}

	bool Save(UObject* Object)
	{
		Object->MarkPackageDirty();
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		const FString Filename = FPackageName::LongPackageNameToFilename(Object->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		return UPackage::SavePackage(Object->GetOutermost(), Object, *Filename, Args);
	}

	UAnimMontage* Montage(const TCHAR* Name, const TCHAR* SequencePath)
	{
		UAnimSequence* Sequence = LoadObject<UAnimSequence>(nullptr, SequencePath);

		if (!Sequence)
		{
			return nullptr;
		}

		UAnimMontage* Result = Asset<UAnimMontage>(Name);
		Result->SetSkeleton(Sequence->GetSkeleton());
		Result->SlotAnimTracks.Reset();
		FSlotAnimationTrack& Track = Result->SlotAnimTracks.AddDefaulted_GetRef();
		Track.SlotName = TEXT("DefaultSlot");
		FAnimSegment Segment;
		Segment.SetAnimReference(Sequence);
		Segment.AnimEndTime = Sequence->GetPlayLength();
		Track.AnimTrack.AnimSegments.Add(Segment);
		Result->SetCompositeLength(Sequence->GetPlayLength());
		Result->CompositeSections.Reset();
		Result->AddAnimCompositeSection(TEXT("Default"), 0.f);
		Result->PostEditChange();
		return Save(Result) ? Result : nullptr;
	}
}
#endif

bool UCCLCombatAssetLibrary::CreateCombatAssets()
{
#if WITH_EDITOR
	UAnimMontage* AttackMontage = Montage(TEXT("AM_UnarmedAttack"), TEXT("/Game/Combat/AS_UnarmedAttack.AS_UnarmedAttack"));
	UAnimMontage* GuardPose = Montage(TEXT("AM_DefensePose"), TEXT("/Game/Combat/AS_UnarmedAttack.AS_UnarmedAttack"));

	if (GuardPose)
	{
		auto& Segment = GuardPose->SlotAnimTracks[0].AnimTrack.AnimSegments[0];
		Segment.AnimStartTime = 0.22f;
		Segment.AnimEndTime = 0.23f;
		GuardPose->SetCompositeLength(0.01f);
		GuardPose->CompositeSections[0].NextSectionName = TEXT("Default");
		if (!Save(GuardPose))
		{
			return false;
		}
	}

	UAnimMontage* DodgeMontage = Montage(TEXT("AM_Dodge"), TEXT("/Game/Combat/AS_Dodge.AS_Dodge"));

	if (!AttackMontage || !GuardPose || !DodgeMontage)
	{
		return false;
	}

	auto* PlayerAttack = Asset<UCCLCombatDefinition>(TEXT("DA_PlayerAttack"));
	PlayerAttack->Montage = AttackMontage;
	PlayerAttack->DamageEffect = UCCLHealthChangeEffect::StaticClass();
	PlayerAttack->MagnitudeTag = CCLTags::Data_Magnitude;
	PlayerAttack->HitRule = UCCLDuelHitRule::StaticClass();
	auto* EnemyAttack = Asset<UCCLCombatDefinition>(TEXT("DA_EnemyAttack"));
	EnemyAttack->DamageEffect = UCCLHealthChangeEffect::StaticClass();
	EnemyAttack->MagnitudeTag = CCLTags::Data_Magnitude;
	EnemyAttack->HitRule = UCCLDuelHitRule::StaticClass();
	EnemyAttack->Cost = 0.f;
	EnemyAttack->Windup = 0.7f;
	EnemyAttack->Recovery = 0.9f;
	EnemyAttack->Montage = AttackMontage;
	auto* PlayerAbilities = Asset<UCCLAbilitySet>(TEXT("DA_PlayerAbilities"));
	PlayerAbilities->Abilities = {
	    {UCCLAttackAbility::StaticClass(), CCLTags::Input_Attack},
	    {UCCLDodgeAbility::StaticClass(), CCLTags::Input_Dodge},
	    {UCCLGuardAbility::StaticClass(), CCLTags::Input_Guard},
	    {UCCLParryAbility::StaticClass(), CCLTags::Input_Parry}};
	auto* EnemyAbilities = Asset<UCCLAbilitySet>(TEXT("DA_EnemyAbilities"));
	EnemyAbilities->Abilities = {{UCCLAttackAbility::StaticClass(), CCLTags::Input_Attack}};
	auto MakeItem = [](const TCHAR* Name, UCCLCombatDefinition* Combat, UCCLAbilitySet* Abilities)
	{
		auto* Item = Asset<UCCLItemDefinition>(Name);
		Item->Fragments.Reset();
		FCCLItemFragment_MeleeWeapon Fragment;
		Fragment.Combat = Combat;
		Fragment.Abilities = Abilities;
		Item->ItemFragments = {FInstancedStruct::Make(Fragment)};
		Item->ItemName = FText::FromString(FCString::Strcmp(Name, TEXT("DA_Unarmed")) == 0 ? TEXT("맨손") : TEXT("적의 맨손"));
		return Save(Item);
	};

	if (!Save(PlayerAttack) || !Save(EnemyAttack) || !Save(PlayerAbilities) || !Save(EnemyAbilities) ||
	    !MakeItem(TEXT("DA_Unarmed"), PlayerAttack, PlayerAbilities) || !MakeItem(TEXT("DA_EnemyUnarmed"), EnemyAttack, EnemyAbilities))
	{
		return false;
	}

	UStateTree* Tree = Asset<UStateTree>(TEXT("ST_MeleeEnemy"));
	auto* Data = NewObject<UStateTreeEditorData>(Tree);
	Tree->EditorData = Data;
	Data->Schema = NewObject<UStateTreeAIComponentSchema>(Data);
	auto& Root = Data->AddRootState();
	auto& Acquire = Root.AddChildState(TEXT("Acquire"));
	auto& Approach = Root.AddChildState(TEXT("Approach"));
	auto& Attack = Root.AddChildState(TEXT("Attack"));
	auto& Return = Root.AddChildState(TEXT("Return"));
	Acquire.AddTask<FCCLEnemyTask>(ECCLEnemyTask::Acquire);
	Approach.AddTask<FCCLEnemyTask>(ECCLEnemyTask::Approach);
	Attack.AddTask<FCCLEnemyTask>(ECCLEnemyTask::Attack);
	Return.AddTask<FCCLEnemyTask>(ECCLEnemyTask::Return);
	Acquire.AddTransition(EStateTreeTransitionTrigger::OnStateSucceeded, EStateTreeTransitionType::GotoState, &Approach);
	Acquire.AddTransition(EStateTreeTransitionTrigger::OnStateFailed, EStateTreeTransitionType::GotoState, &Return);
	Approach.AddTransition(EStateTreeTransitionTrigger::OnStateSucceeded, EStateTreeTransitionType::GotoState, &Attack);
	Approach.AddTransition(EStateTreeTransitionTrigger::OnStateFailed, EStateTreeTransitionType::GotoState, &Acquire);
	Attack.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::GotoState, &Acquire);
	Return.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::GotoState, &Acquire);
	FStateTreeCompilerLog Log;
	FStateTreeCompiler Compiler(Log);

	if (!Compiler.Compile(*Tree))
	{
		return false;
	}

	return Save(Tree);
#else
	return false;
#endif
}

bool UCCLCombatAssetLibrary::ConfigureCombatWorld()
{
#if WITH_EDITOR
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;

	if (!World || !World->GetWorldPartition())
	{
		return false;
	}

	World->GetWorldPartition()->SetEnableStreaming(false);
	ANavMeshBoundsVolume* Bounds = nullptr;
	for (TActorIterator<ANavMeshBoundsVolume> It(World); It; ++It)
	{
		if (It->GetActorLabel() == TEXT("CombatNavigation"))
		{
			Bounds = *It;
			break;
		}
	}
	if (!Bounds)
	{
		Bounds = World->SpawnActor<ANavMeshBoundsVolume>();
	}
	if (!Bounds)
	{
		return false;
	}
	auto* Builder = NewObject<UCubeBuilder>(Bounds);
	Builder->X = 4000.f;
	Builder->Y = 4000.f;
	Builder->Z = 600.f;

	UActorFactory::CreateBrushForVolumeActor(Bounds, Builder);

	Bounds->SetActorLabel(TEXT("CombatNavigation"));
	Bounds->SetActorLocation(FVector(0.f, 0.f, 150.f));
	Bounds->GetRootComponent()->UpdateBounds();
	if (Bounds->GetComponentsBoundingBox(true).GetExtent().IsNearlyZero())
	{
		return false;
	}
	Bounds->MarkPackageDirty();

	if (auto* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
	{
		Navigation->OnNavigationBoundsUpdated(Bounds);
		Navigation->Build();
	}

	return true;
#else
	return false;
#endif
}
