#include "CCLPlayerState.h"
#include "Campaign/CCLCampaignDirector.h"
#include "Campaign/CCLCampaignState.h"
#include "Items/CCLWorldPickup.h"
#include "EngineUtils.h"
#include "AbilitySystem/CCLAbilitySystemComponent.h"
#include "AbilitySystem/CCLEffects.h"
#include "AbilitySystem/CCLGameplayTags.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "Combat/CCLCombatDefinition.h"
#include "Combat/CCLEnemyCharacter.h"
#include "Combat/CCLEnemyDefinition.h"
#include "Combat/CCLFighterComponent.h"
#include "Combat/CCLHitRule.h"
#include "Environment/CCLWorldSnapshot.h"
#include "Agents/CCLLifeSimulation.h"
#include "Items/CCLInventoryComponent.h"
#include "Items/CCLItemCatalog.h"
#include "Items/CCLItemDefinition.h"
#include "Items/CCLLoadoutComponent.h"
#include "Items/CCLSkillDefinition.h"
#include "AbilitySystem/CCLOffenseSet.h"
#include "Environment/CCLSurfacePublication.h"
#include "KSIWorldParticipant.h"
#include "Misc/ScopeExit.h"
#include "Session/CCLProfileIdentity.h"
#include "Session/CCLSessionRecord.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"

namespace
{
	// A standalone game world for rules that need Actors, components and gameplay effects.
	struct FCCLRuleWorld
	{
		FCCLRuleWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("CCLRuleTestWorld"));
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();

			// Without a game mode, world settings start play so spawned Actors receive BeginPlay.
			if (!World->HasBegunPlay())
			{
				World->GetWorldSettings()->NotifyBeginPlay();
			}
		}

		~FCCLRuleWorld()
		{
			World->EndPlay(EEndPlayReason::Quit);
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}

		ACCLEnemyCharacter* SpawnEnemy(const FVector& Location, float Yaw, UCCLEnemyDefinition* Definition = nullptr) const
		{
			const FTransform Transform(FRotator(0.f, Yaw, 0.f), Location);
			auto* Enemy = World->SpawnActorDeferred<ACCLEnemyCharacter>(ACCLEnemyCharacter::StaticClass(), Transform, nullptr, nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (Definition)
			{
				Enemy->ApplyDefinition(Definition);
			}

			Enemy->bRespawnEnabled = 0;
			Enemy->FinishSpawning(Transform);
			return Enemy;
		}

		UWorld* World = nullptr;
	};

	float Health(const AActor* Actor)
	{
		const auto* Enemy = Cast<ACCLEnemyCharacter>(Actor);
		return Enemy->GetAbilitySystemComponent()->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute());
	}

	FKSIHitContext Hit(ACCLEnemyCharacter* Source, ACCLEnemyCharacter* Target, const UCCLCombatDefinition* Definition)
	{
		FKSIHitContext Context;
		Context.Source = Source;
		Context.Target = Target;
		Context.SourceASC = Source->GetAbilitySystemComponent();
		Context.TargetASC = Target->GetAbilitySystemComponent();
		Context.Definition = Definition;
		Context.Hit.ImpactPoint = Target->GetActorLocation();
		return Context;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLHitOutcomeTest, "CCL.Combat.HitRule.Outcomes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLHitOutcomeTest::RunTest(const FString& Parameters)
{
	FCCLRuleWorld Fixture;
	auto* Source = Fixture.SpawnEnemy(FVector::ZeroVector, 0.f);
	auto* Target = Fixture.SpawnEnemy(FVector(100.f, 0.f, 0.f), 180.f);
	auto* TargetFighter = Target->FindComponentByClass<UCCLFighterComponent>();
	const auto* Attack = Source->FindComponentByClass<UCCLFighterComponent>()->GetAttack();
	if (!TestNotNull(TEXT("unarmed attack definition"), Attack) || !TestNotNull(TEXT("hit rule"), Attack->HitRule.Get()))
	{
		return false;
	}

	const float Full = Health(Target);
	TestEqual(TEXT("same team is rejected"), KSIHit::Apply(Hit(Source, Target, Attack)), FGameplayTag(CCLTags::Outcome_Rejected));
	TestEqual(TEXT("rejected hit keeps health"), Health(Target), Full);

	TargetFighter->Team = 0;
	auto* TargetASC = Cast<UCCLAbilitySystemComponent>(Target->GetAbilitySystemComponent());
	const auto Invulnerable = TargetASC->ApplyEffect(UCCLInvulnerableEffect::StaticClass());
	TestEqual(TEXT("invulnerable target evades"), KSIHit::Apply(Hit(Source, Target, Attack)), FGameplayTag(CCLTags::Outcome_Evaded));
	TestEqual(TEXT("evaded hit keeps health"), Health(Target), Full);
	TargetASC->RemoveActiveGameplayEffect(Invulnerable);

	TestEqual(TEXT("open target takes damage"), KSIHit::Apply(Hit(Source, Target, Attack)), FGameplayTag(CCLTags::Outcome_Damage));
	TestEqual(TEXT("damage equals the attack"), Health(Target), Full - Attack->Damage);
	TestEqual(TEXT("damage commits its feedback"), TargetFighter->GetFeedback(), FString(TEXT("피격")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLHitRejectedEffectTest, "CCL.Combat.HitRule.RejectedEffectCommitsNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLHitRejectedEffectTest::RunTest(const FString& Parameters)
{
	FCCLRuleWorld Fixture;
	auto* Source = Fixture.SpawnEnemy(FVector::ZeroVector, 0.f);
	auto* Target = Fixture.SpawnEnemy(FVector(100.f, 0.f, 0.f), 180.f);
	auto* TargetFighter = Target->FindComponentByClass<UCCLFighterComponent>();
	TargetFighter->Team = 0;
	const auto* Attack = Source->FindComponentByClass<UCCLFighterComponent>()->GetAttack();
	if (!TestNotNull(TEXT("unarmed attack definition"), Attack))
	{
		return false;
	}

	auto* Broken = DuplicateObject(Attack, GetTransientPackage());
	Broken->MagnitudeTag = FGameplayTag();

	// The rule decides Damage, but the damage effect cannot be built. No stagger, cancel or feedback may remain.
	const float Full = Health(Target);
	TestFalse(TEXT("unappliable damage has no outcome"), KSIHit::Apply(Hit(Source, Target, Broken)).IsValid());
	TestEqual(TEXT("health unchanged"), Health(Target), Full);
	TestTrue(TEXT("no feedback was committed"), TargetFighter->GetFeedback().IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLEnemyPatternTest, "CCL.Combat.EnemyPatterns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLEnemyPatternTest::RunTest(const FString& Parameters)
{
	auto* Warden = LoadObject<UCCLEnemyDefinition>(nullptr, TEXT("/Game/Progression/DA_GateWarden.DA_GateWarden"));
	if (!TestNotNull(TEXT("warden definition"), Warden) || !TestEqual(TEXT("two patterns"), Warden->Patterns.Num(), 2))
	{
		return false;
	}

	TArray<FText> Errors;
	TestTrue(TEXT("warden definition validates"), Warden->ValidateDefinition(Errors));
	FCCLRuleWorld Fixture;
	auto* Enemy = Fixture.SpawnEnemy(FVector::ZeroVector, 0.f, Warden);
	const auto* Fighter = Enemy->FindComponentByClass<UCCLFighterComponent>();
	const auto* Sweep = Warden->Patterns[0].Attack.Get();
	const auto* Heavy = Warden->Patterns[1].Attack.Get();
	TestEqual(TEXT("definition sets display name"), Enemy->DisplayName, Warden->DisplayName.ToString());
	TestEqual(TEXT("definition sets health"), Health(Enemy), Warden->InitialHealth);
	Enemy->SelectAttackPattern();
	TestTrue(TEXT("full health uses the heavy attack only"), Fighter->AttackOverride == Heavy);
	Enemy->GetAbilitySystemComponent()->SetNumericAttributeBase(UCCLHealthSet::GetHealthAttribute(), Warden->InitialHealth * 0.5f);
	Enemy->SelectAttackPattern();
	TestTrue(TEXT("half health unlocks the sweep"), Fighter->AttackOverride == Sweep);
	Enemy->SelectAttackPattern();
	TestTrue(TEXT("unlocked patterns alternate"), Fighter->AttackOverride == Heavy);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLInventoryCoreTest, "CCL.Items.Inventory.AddMoveRemove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLInventoryCoreTest::RunTest(const FString& Parameters)
{
	const auto* Catalog = UCCLItemCatalog::Get();
	auto* Potion = Catalog ? Catalog->FindItem(TEXT("DA_RecoveryPotion")) : nullptr;
	auto* Sword = Catalog ? Catalog->FindItem(TEXT("DA_TrainingSword")) : nullptr;
	if (!TestNotNull(TEXT("catalog potion"), Potion) || !TestNotNull(TEXT("catalog sword"), Sword))
	{
		return false;
	}

	FCCLRuleWorld Fixture;
	auto* Player = Fixture.World->SpawnActor<ACCLPlayerState>();
	auto* Inventory = Player->GetInventory();
	Inventory->Capacity = 2;
	const FGuid First = Inventory->Add(Potion, 1);
	TestTrue(TEXT("stackable item stacks into the same entry"), First.IsValid() && Inventory->Add(Potion, 1) == First);
	const FGuid Blade = Inventory->Add(Sword, 1);
	TestTrue(TEXT("second slot used"), Blade.IsValid() && Inventory->Find(Blade)->Slot == 1);
	TestFalse(TEXT("full bag rejects a new entry"), Inventory->CanAdd(Sword, 1));
	TestTrue(TEXT("move swaps occupied slots"), Inventory->MoveToSlot(Blade, 0)
		&& Inventory->Find(Blade)->Slot == 0 && Inventory->Find(First)->Slot == 1);
	TestFalse(TEXT("move outside capacity is rejected"), Inventory->MoveToSlot(Blade, 5));
	TestFalse(TEXT("removing more than held is rejected"), Inventory->Remove(First, 3));
	TestTrue(TEXT("partial remove keeps the entry"), Inventory->Remove(First, 1) && Inventory->Find(First)->Quantity == 1);
	TestTrue(TEXT("full remove deletes the entry"), Inventory->Remove(First, 1) && !Inventory->Find(First));
	TestTrue(TEXT("freed slot accepts a new entry"), Inventory->CanAdd(Sword, 1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLLoadoutUseTest, "CCL.Items.Loadout.EquipAndUse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLLoadoutUseTest::RunTest(const FString& Parameters)
{
	const auto* Catalog = UCCLItemCatalog::Get();
	auto* Potion = Catalog ? Catalog->FindItem(TEXT("DA_RecoveryPotion")) : nullptr;
	auto* Sword = Catalog ? Catalog->FindItem(TEXT("DA_TrainingSword")) : nullptr;
	if (!TestNotNull(TEXT("catalog potion"), Potion) || !TestNotNull(TEXT("catalog sword"), Sword))
	{
		return false;
	}

	FCCLRuleWorld Fixture;
	auto* Player = Fixture.World->SpawnActor<ACCLPlayerState>();
	auto* ASC = Player->GetCCLAbilitySystem();
	ASC->InitAbilityActorInfo(Player, Player);
	auto* Inventory = Player->GetInventory();
	auto* Loadout = Player->GetLoadout();
	const FGuid Potions = Inventory->Add(Potion, 2);
	const FGuid Blade = Inventory->Add(Sword, 1);

	TestFalse(TEXT("full health refuses a heal"), Loadout->Use(Potions));
	TestEqual(TEXT("refused use keeps the item"), Inventory->Find(Potions)->Quantity, 2);
	ASC->SetNumericAttributeBase(UCCLHealthSet::GetHealthAttribute(), 10.f);
	TestTrue(TEXT("use heals"), Loadout->Use(Potions));
	TestTrue(TEXT("one use consumes one item and heals once"), Inventory->Find(Potions)->Quantity == 1
		&& ASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) > 10.f);

	TestTrue(TEXT("equip to the right hand"), Loadout->Equip(Blade, CCLItemTags::Slot_RightHand) && Loadout->IsEquipped(Blade)
		&& Inventory->Find(Blade)->Slot == INDEX_NONE);
	TestTrue(TEXT("unequip returns the item to the bag"), Loadout->Unequip(Blade) && !Loadout->IsEquipped(Blade)
		&& Inventory->Find(Blade)->Slot >= 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLSessionCodecRuleTest, "CCL.Session.Codec.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLSessionCodecRuleTest::RunTest(const FString& Parameters)
{
	FCCLSessionRecord Record;
	FCCLSavedItem Potion;
	Potion.Id = FGuid::NewGuid();
	Potion.Definition = TEXT("DA_RecoveryPotion");
	Potion.Quantity = 1;
	Potion.Slot = 0;
	Record.Items.Add(Potion);
	Record.SupplyPlacements = {
		{FGuid(1, 0, 0, 1), TEXT("DA_Bullets"), 10},
		{FGuid(1, 0, 0, 2), TEXT("DA_Bullets"), 60}};
	TestTrue(TEXT("repeated supplies are separate pickups"), FCCLSessionCodec::Validate(Record));

	TArray<uint8> Bytes;
	FCCLSessionRecord Decoded;
	if (!TestTrue(TEXT("encode"), FCCLSessionCodec::Encode(Record, Bytes)) || !TestTrue(TEXT("decode"), FCCLSessionCodec::Decode(Bytes, Decoded)))
	{
		return false;
	}

	TestEqual(TEXT("supply multiset survives"), Decoded.SupplyPlacements.Num(), 2);
	TestEqual(TEXT("placement quantity survives"), Decoded.SupplyPlacements[1].Quantity, 60);
	TestEqual(TEXT("placement identity survives"), Decoded.SupplyPlacements[0].PlacementId, Record.SupplyPlacements[0].PlacementId);
	auto Duplicate = Record;
	Duplicate.SupplyPlacements[1].PlacementId = Duplicate.SupplyPlacements[0].PlacementId;
	TestFalse(TEXT("duplicate placements rejected"), FCCLSessionCodec::Validate(Duplicate));
	Duplicate = Record;
	Duplicate.SupplyPlacements[0].Quantity = 0;
	TestFalse(TEXT("empty quantity rejected"), FCCLSessionCodec::Validate(Duplicate));
	auto Legacy = Record;
	Legacy.Version = 7;
	Legacy.SupplyPlacements.Reset();
	Legacy.RemainingSupplies = {TEXT("DA_Bullets")};
	TArray<uint8> LegacyBytes;
	TestTrue(TEXT("legacy names kept for map-aware migration"), FCCLSessionCodec::Encode(Legacy, LegacyBytes) &&
		FCCLSessionCodec::Decode(LegacyBytes, Decoded) && Decoded.Version == 7 && Decoded.RemainingSupplies.Num() == 1);
	auto Tampered = Bytes;
	Tampered.Last() ^= 0x5A;
	TestFalse(TEXT("checksum rejects tampering"), FCCLSessionCodec::Decode(Tampered, Decoded));
	auto WrongMagic = Bytes;
	WrongMagic[0] = 'X';
	TestFalse(TEXT("format tag is required"), FCCLSessionCodec::Decode(WrongMagic, Decoded));

	auto Unknown = Record;
	Unknown.Items[0].Definition = TEXT("DA_NotInCatalog");
	TestFalse(TEXT("unlisted definition is rejected"), FCCLSessionCodec::Validate(Unknown));
	auto Victory = Record;
	Victory.Victory = 1;
	TestFalse(TEXT("victory needs defeated guards"), FCCLSessionCodec::Validate(Victory));
	Victory.DefeatedGuards = 0x07;
	TestTrue(TEXT("guard mask follows the road length"), FCCLSessionCodec::Validate(Victory));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLProfileIdentityTest, "CCL.Session.ProfileIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLProfileIdentityTest::RunTest(const FString& Parameters)
{
	const FString Secret = CCLProfileIdentity::IssueSecret();
	const FString Other = CCLProfileIdentity::IssueSecret();
	TestTrue(TEXT("issued secret is valid"), CCLProfileIdentity::IsValidSecret(Secret));
	TestNotEqual(TEXT("secrets are unique"), Secret, Other);
	TestFalse(TEXT("short secret is invalid"), CCLProfileIdentity::IsValidSecret(TEXT("abc")));
	TestFalse(TEXT("uppercase secret is invalid"), CCLProfileIdentity::IsValidSecret(Secret.ToUpper()));
	const FString Key = CCLProfileIdentity::IdentityKeyFor(Secret);
	TestEqual(TEXT("identity is stable"), Key, CCLProfileIdentity::IdentityKeyFor(Secret));
	TestNotEqual(TEXT("identity differs per secret"), Key, CCLProfileIdentity::IdentityKeyFor(Other));
	TestFalse(TEXT("saved identity does not reveal the secret"), Key.Contains(Secret));
	TestTrue(TEXT("identity fits the profile key format"), Key.StartsWith(TEXT("profile:")) && Key.Len() == 48);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLWorldSectionRuleTest, "CCL.Environment.Save.SectionRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLWorldSectionRuleTest::RunTest(const FString& Parameters)
{
	FCCLWorldSnapshot Snapshot;
	Snapshot.SetSection(TEXT("Players"), {1});
	Snapshot.SetSection(TEXT("Inventories"), {2});
	TestTrue(TEXT("sections stay sorted by name"), Snapshot.Sections.Num() == 2 && Snapshot.Sections[0].Name == FName(TEXT("Inventories")));
	Snapshot.SetSection(TEXT("Players"), {3});
	TestTrue(TEXT("setting an existing section replaces it"), Snapshot.Sections.Num() == 2 && *Snapshot.FindSection(TEXT("Players")) == TArray<uint8>{3});
	Snapshot.RemoveSection(TEXT("Players"));
	TestNull(TEXT("removed section is absent"), Snapshot.FindSection(TEXT("Players")));

	// Validation runs on encode; an unregistered section cannot be validated and is refused.
	FString Error;
	FCCLLifeSimulation Life;
	FKSIWorldClock Clock;
	FCCLWorldIdentity Identity;
	Identity.WorldId = FGuid::NewGuid();
	FCCLWorldSnapshot Valid;
	TArray<uint8> Bytes;
	if (!TestTrue(TEXT("world fixture"), Life.Initialize(FCCLLifeSimulation::MerchantScenario(42), Error)
		&& Clock.Reset(Life.GetTime(), 60., Error) && FCCLWorldSnapshotCodec::Capture(Identity, Clock, Life, Valid, Error)
		&& FCCLWorldSnapshotCodec::Encode(Valid, Bytes, Error)))
	{
		return false;
	}

	auto Unknown = Valid;
	Unknown.SetSection(TEXT("UnregisteredSection"), {1});
	TestFalse(TEXT("unregistered section is refused"), FCCLWorldSnapshotCodec::Encode(Unknown, Bytes, Error));
	auto Corrupt = Valid;
	Corrupt.SetSection(TEXT("Players"), {1, 2, 3});
	TestFalse(TEXT("registered section validates its own format"), FCCLWorldSnapshotCodec::Encode(Corrupt, Bytes, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLCampaignIdentityTest, "CCL.Campaign.StableIdentities",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLCampaignIdentityTest::RunTest(const FString& Parameters)
{
	for (int32 Count : {1, 2, 3, 8})
	{
		TSet<FGuid> Ids{ACCLCampaignDirector::BossAgentId()};
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FGuid Id = ACCLCampaignDirector::MakeGuardAgentId(Index);
			TestTrue(TEXT("guard identity valid and distinct from boss"), Id.IsValid() && !Ids.Contains(Id));
			Ids.Add(Id);
		}
	}

	TestEqual(TEXT("first legacy identity"), ACCLCampaignDirector::MakeGuardAgentId(0), FGuid(0xCC190000, 0, 1, 1));
	TestEqual(TEXT("second legacy identity"), ACCLCampaignDirector::MakeGuardAgentId(1), FGuid(0xCC190000, 0, 1, 2));
	TestFalse(TEXT("invalid guard index"), ACCLCampaignDirector::MakeGuardAgentId(8).IsValid());
	TestFalse(TEXT("negative guard index"), ACCLCampaignDirector::MakeGuardAgentId(-1).IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLCampaignSupplyTest, "CCL.Campaign.SupplyPlacementRestore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLCampaignSupplyTest::RunTest(const FString& Parameters)
{
	FCCLRuleWorld TestWorld;
	TestWorld.World->SetGameState(TestWorld.World->SpawnActor<ACCLCampaignState>());
	const auto* Defaults = GetDefault<ACCLCampaignDirector>();
	TArray<FCCLSupplySpawn> Layout = {Defaults->Supplies[2], Defaults->Supplies[2]};
	Layout[0].PlacementId = FGuid(19, 2, 0, 1);
	Layout[0].Quantity = 10;
	Layout[0].Location = FVector(10000., 0., 0.);
	Layout[1].PlacementId = FGuid(19, 2, 0, 2);
	Layout[1].Quantity = 60;
	Layout[1].Location = FVector(10200., 0., 0.);
	int32 Run = 0;
	auto SpawnDirector = [&](const TArray<FCCLSupplySpawn>& Supplies)
	{
		auto* Director = TestWorld.World->SpawnActorDeferred<ACCLCampaignDirector>(ACCLCampaignDirector::StaticClass(), FTransform::Identity);
		Director->Supplies = Supplies;
		Director->Guards.SetNum(1);
		Director->Guards[0].Location = FVector(20000. + 1000. * ++Run, 0., 96.);
		Director->FinishSpawning(FTransform::Identity);
		return Director;
	};

	for (int32 Collected = 0; Collected < 2; ++Collected)
	{
		auto* Original = SpawnDirector(Layout);
		for (TActorIterator<ACCLWorldPickup> It(TestWorld.World); It; ++It)
		{
			if (It->GetOwner() == Original && It->GetActorLocation() == Layout[Collected].Location)
			{
				It->Destroy();
			}
		}

		FString Error;
		FCCLSessionRecord Record;
		if (!TestTrue(TEXT("capture remaining placement"), Original->CaptureSupplies(Record.SupplyPlacements, Error)))
		{
			return false;
		}

		TestEqual(TEXT("only one placement remains"), Record.SupplyPlacements.Num(), 1);
		TArray<uint8> Bytes;
		FCCLSessionRecord Decoded;
		if (!TestTrue(TEXT("placement checkpoint round trip"), FCCLSessionCodec::Encode(Record, Bytes) && FCCLSessionCodec::Decode(Bytes, Decoded)))
		{
			return false;
		}

		TArray<FCCLSupplySpawn> Reordered = {Layout[1], Layout[0]};
		auto* Restored = SpawnDirector(Reordered);
		TArray<FCCLSavedSupply> Prepared;
		auto Invalid = Decoded;
		Invalid.SupplyPlacements[0].PlacementId = FGuid::NewGuid();
		TestFalse(TEXT("unknown placement rejected before mutation"), Restored->PrepareSupplyRestore(Invalid, Prepared, Error));
		if (!TestTrue(TEXT("reordered layout prepares"), Restored->PrepareSupplyRestore(Decoded, Prepared, Error)))
		{
			return false;
		}

		Restored->ApplySupplies(Prepared);
		int32 Remaining = 0;
		for (TActorIterator<ACCLWorldPickup> It(TestWorld.World); It; ++It)
		{
			if (It->GetOwner() == Restored && !It->IsActorBeingDestroyed())
			{
				++Remaining;
				TestEqual(TEXT("correct position"), It->GetActorLocation(), Layout[1 - Collected].Location);
				TestEqual(TEXT("correct quantity"), It->Quantity, Layout[1 - Collected].Quantity);
			}
		}

		TestEqual(TEXT("one restored pickup"), Remaining, 1);
		FCCLSessionRecord Legacy;
		Legacy.Version = 7;
		Legacy.RemainingSupplies = {TEXT("DA_Bullets")};
		TestFalse(TEXT("ambiguous legacy layout rejected"), Restored->PrepareSupplyRestore(Legacy, Prepared, Error));
	}

	auto* DefaultDirector = SpawnDirector(Defaults->Supplies);
	FCCLSessionRecord Legacy;
	Legacy.Version = 7;
	Legacy.RemainingSupplies = {TEXT("DA_Bullets")};
	FString Error;
	TArray<FCCLSavedSupply> Prepared;
	TestTrue(TEXT("known legacy layout migrates"), DefaultDirector->PrepareSupplyRestore(Legacy, Prepared, Error));
	TestTrue(TEXT("legacy quantity and placement"), Prepared.Num() == 1 && Prepared[0].Quantity == 60 && Prepared[0].PlacementId == Defaults->Supplies[2].PlacementId);
	DefaultDirector->Guards.SetNum(3);
	TestFalse(TEXT("legacy colliding guards rejected"), DefaultDirector->PrepareSupplyRestore(Legacy, Prepared, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLLoadoutPublicationTest, "CCL.Items.Loadout.AtomicPublication",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLLoadoutPublicationTest::RunTest(const FString& Parameters)
{
	const auto* Catalog = UCCLItemCatalog::Get();
	auto* Sword = Catalog ? Catalog->FindItem(TEXT("DA_IronGauntlets")) : nullptr;
	auto* Skill = Catalog ? Catalog->FindSkill(TEXT("DA_PowerTraining")) : nullptr;
	if (!TestNotNull(TEXT("equipment"), Sword) || !TestNotNull(TEXT("skill"), Skill))
	{
		return false;
	}
	FCCLRuleWorld Fixture;
	auto* Player = Fixture.World->SpawnActor<ACCLPlayerState>();
	auto* ASC = Player->GetCCLAbilitySystem();
	ASC->InitAbilityActorInfo(Player, Player);
	auto* Inventory = Player->GetInventory();
	auto* Loadout = Player->GetLoadout();
	Loadout->GrantPoints(10);
	const FGuid Item = Inventory->Add(Sword, 1);
	const float BaseAttack = ASC->GetNumericAttribute(UCCLOffenseSet::GetAttackBonusAttribute());
	const int32 BagSlot = Inventory->Find(Item)->Slot;
	int32 Notifications = 0;
	bool bExpectEquipped = true;
	const auto Changed = Inventory->OnChanged.AddLambda([&]()
	{
		++Notifications;
		TestEqual(TEXT("equipment committed before inventory notification"), Loadout->IsEquipped(Item), bExpectEquipped);
		const auto* Entry = Inventory->Find(Item);
		TestTrue(TEXT("placement committed before notification"), Entry && (bExpectEquipped ? Entry->Slot == INDEX_NONE : Entry->Slot == BagSlot));
		TestEqual(TEXT("effect committed before notification"), ASC->GetNumericAttribute(UCCLOffenseSet::GetAttackBonusAttribute()),
			BaseAttack + (bExpectEquipped ? Sword->FindFragment<FCCLItemFragment_Equip>()->Magnitude : 0.f));
		TestFalse(TEXT("equip notification cannot reenter unequip"), Loadout->Unequip(Item));
		TestFalse(TEXT("notification cannot reenter training"), Loadout->Learn(Skill));
		TestFalse(TEXT("notification cannot mutate inventory directly"), Inventory->Remove(Item, 1));
	});
	ON_SCOPE_EXIT
	{
		Inventory->OnChanged.Remove(Changed);
	};
	TestTrue(TEXT("equip succeeds"), Loadout->Equip(Item));
	TestEqual(TEXT("equip publishes once"), Notifications, 1);
	bExpectEquipped = false;
	TestTrue(TEXT("unequip succeeds"), Loadout->Unequip(Item, BagSlot));
	TestEqual(TEXT("unequip publishes once"), Notifications, 2);
	Inventory->OnChanged.Remove(Changed);

	bool bReject = true;
	int32 QueryCalls = 0;
	FGameplayEffectApplicationQuery Query = FGameplayEffectApplicationQuery::CreateLambda(
		[&](const FActiveGameplayEffectsContainer&, const FGameplayEffectSpec&)
		{
			++QueryCalls;
			TestFalse(TEXT("GAS query cannot reenter equip"), Loadout->Equip(Item));
			TestFalse(TEXT("GAS query cannot remove staged item"), Inventory->Remove(Item, 1));
			TestFalse(TEXT("GAS query cannot add an item"), Inventory->Add(Sword, 1).IsValid());
			TestFalse(TEXT("GAS query cannot reenter training"), Loadout->Learn(Skill));
			return !bReject;
		});
	const auto QueryHandle = Query.GetHandle();
	ASC->GameplayEffectApplicationQueries.Add(Query);
	ON_SCOPE_EXIT
	{
		ASC->GameplayEffectApplicationQueries.RemoveAll([&](const auto& Value) { return Value.GetHandle() == QueryHandle; });
	};
	TestFalse(TEXT("GAS rejection refuses equip"), Loadout->Equip(Item));
	TestTrue(TEXT("rejection preserves bag and effects"), !Loadout->IsEquipped(Item) && Inventory->Find(Item)->Slot == BagSlot
		&& ASC->GetNumericAttribute(UCCLOffenseSet::GetAttackBonusAttribute()) == BaseAttack && !Inventory->IsTransactionOpen());
	bReject = false;
	const int32 Points = Loadout->GetPoints();
	TestTrue(TEXT("learn once through synchronous callback"), Loadout->Learn(Skill));
	TestTrue(TEXT("query actually ran for both effects"), QueryCalls == 2);
	TestEqual(TEXT("single point debit"), Loadout->GetPoints(), Points - Skill->PointCost);
	TestFalse(TEXT("already learned refused"), Loadout->Learn(Skill));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLCatalogIdentityTest, "CCL.Items.Catalog.UniqueSaveNames",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLCatalogIdentityTest::RunTest(const FString& Parameters)
{
	auto* Catalog = NewObject<UCCLItemCatalog>();
	auto* OuterA = NewObject<UCCLItemCatalog>();
	auto* OuterB = NewObject<UCCLItemCatalog>();
	auto* A = NewObject<UCCLItemDefinition>(OuterA, TEXT("DuplicateName"));
	auto* B = NewObject<UCCLItemDefinition>(OuterB, TEXT("DuplicateName"));
	Catalog->Items = {A};
	TArray<FText> Errors;
	TestTrue(TEXT("unique item accepted"), Catalog->ValidateCatalog(Errors) && Catalog->FindItem(A->GetFName()) == A);
	Catalog->Items.Add(B);
	TestFalse(TEXT("same name in different packages rejected"), Catalog->ValidateCatalog(Errors));
	TestNull(TEXT("ambiguous lookup never picks first"), Catalog->FindItem(A->GetFName()));
	Catalog->Items = {A, A};
	TestFalse(TEXT("duplicate entry also rejected"), Catalog->ValidateCatalog(Errors));
	Catalog->Items = {A, nullptr};
	TestFalse(TEXT("empty reference rejected"), Catalog->ValidateCatalog(Errors));
	Catalog->Items = {A};
	auto* SkillA = NewObject<UCCLSkillDefinition>(OuterA, TEXT("DuplicateSkill"));
	auto* SkillB = NewObject<UCCLSkillDefinition>(OuterB, TEXT("DuplicateSkill"));
	Catalog->TrainingSkills = {SkillA, SkillB};
	TestFalse(TEXT("duplicate skill rejected"), Catalog->ValidateCatalog(Errors));
	TestNull(TEXT("ambiguous skill unresolved"), Catalog->FindSkill(SkillA->GetFName()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLSurfacePublicationTest, "CCL.Environment.Surface.SharedPublication",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLSurfacePublicationTest::RunTest(const FString& Parameters)
{
	FKSISurfaceSimulation Surface;
	FString Error;
	const FGuid Epoch = FGuid::NewGuid();
	if (!TestTrue(TEXT("surface fixture"), Surface.Initialize(FGuid::NewGuid(), 0., 0., 0, Error)))
	{
		return false;
	}
	FCCLSurfacePublicationCache Cache;
	const auto First = Cache.Capture(Surface, Epoch, {}, Error);
	if (!TestTrue(TEXT("first immutable publication"), First.IsValid()))
	{
		return false;
	}
	for (int32 Connection = 0; Connection < 8; ++Connection)
	{
		TestTrue(TEXT("eight consumers share unchanged state and bytes"), First == Cache.Capture(Surface, Epoch, {}, Error));
	}
	const auto NextEpoch = Cache.Capture(Surface, FGuid::NewGuid(), {}, Error);
	TestTrue(TEXT("restore epoch starts distinct history even with equal bytes"), NextEpoch && NextEpoch != First && First->Epoch == Epoch);
	TMap<FGuid, uint64> Serials;
	Serials.Add(FGuid::NewGuid(), 1);
	const auto Terrain = Cache.Capture(Surface, NextEpoch->Epoch, Serials, Error);
	TestTrue(TEXT("terrain completion serial creates a new publication"), Terrain && Terrain != NextEpoch && NextEpoch->TerrainSerials.IsEmpty());
	FKSISurfaceSimulation Invalid;
	TestFalse(TEXT("invalid candidate not published"), Cache.Capture(Invalid, Epoch, {}, Error).IsValid());
	TestTrue(TEXT("failed capture keeps valid publication"), Terrain == Cache.Capture(Surface, Terrain->Epoch, Serials, Error));
	TestTrue(TEXT("older bytes still decode while consumers retain them"), Invalid.Restore(First->Bytes, Error));
	return true;
}

// Compile-time regression: save participation has no mandatory environment-time API.
namespace
{
	class FCCLSaveOnlyParticipant final : public IKSIWorldParticipant
	{
	public:
		virtual FName GetWorldSection() const override { return TEXT("SaveOnly"); }
		virtual int32 GetWorldOrder() const override { return 0; }
		virtual bool CaptureWorldSection(FKSIWorldCaptureContext&, TArray<uint8>&, FString&) override { return true; }
		virtual bool PrepareWorldRestore(FKSIWorldRestoreContext&, const TArray<uint8>*, FString&) override { return true; }
		virtual bool CommitWorldRestore(TFunctionRef<bool()> CommitInner) override { return CommitInner(); }
		virtual void PublishWorldRestore() override {}
		virtual void CancelWorldRestore() override {}
	};
	static_assert(!TIsAbstract<FCCLSaveOnlyParticipant>::Value);
}
