#include "CCLCampaignSmokeSubsystem.h"
#include "Campaign/CCLExpeditionComponent.h"
#include "Campaign/CCLVillageSteward.h"
#include "Campaign/CCLLifeVillager.h"
#include "Combat/CCLCombatDefinition.h"
#include "Combat/CCLEnemyDefinition.h"
#include "AbilitySystem/CCLEffects.h"

#include "CCLCharacter.h"
#include "CCLPlayerController.h"
#include "CCLTestBridgeComponent.h"
#include "CCLPlayerState.h"
#include "Items/CCLInventoryComponent.h"
#include "Items/CCLItemDefinition.h"
#include "Items/CCLLoadoutComponent.h"
#include "Items/CCLWorldPickup.h"
#include "Items/CCLSkillDefinition.h"
#include "Session/CCLSessionRecord.h"
#include "AbilitySystem/CCLOffenseSet.h"
#include "Combat/CCLHealthTarget.h"
#include "KSICombatComponent.h"
#include "Combat/CCLFighterComponent.h"
#include "Campaign/CCLCampaignDirector.h"
#include "Campaign/CCLCampaignState.h"
#include "Combat/CCLEnemyCharacter.h"
#include "AbilitySystem/CCLAbilitySystemComponent.h"
#include "AbilitySystem/CCLGameplayTags.h"
#include "AbilitySystem/CCLStaminaSet.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "NavigationSystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

bool UCCLCampaignSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	FString Role;
	const auto* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && FParse::Value(FCommandLine::Get(), TEXT("CCLCampaignSmoke="), Role);
#endif
}

TStatId UCCLCampaignSmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLCampaignSmokeSubsystem, STATGROUP_Tickables);
}

void UCCLCampaignSmokeSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bFailed || (bComplete && (GetWorld()->GetNetMode() == NM_DedicatedServer || bClientReported)))
	{
		return;
	}

	if (!Started)
	{
		Started = FPlatformTime::Seconds();
	}
	if (FPlatformTime::Seconds() - Started > 180.)
	{
		Check(false, TEXT("watchdog"));
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	auto* State = GetWorld()->GetGameState<ACCLCampaignState>();
	auto* Local = Cast<ACCLPlayerController>(GetWorld()->GetFirstPlayerController());
	auto* Pawn = Local ? Cast<ACCLCharacter>(Local->GetPawn()) : nullptr;
	auto* ASC = Pawn ? Cast<UCCLAbilitySystemComponent>(Pawn->GetAbilitySystemComponent()) : nullptr;
	FString Role;
	FParse::Value(FCommandLine::Get(), TEXT("CCLCampaignSmoke="), Role);
	if (State && Local && Local->IsLocalController() && Pawn &&
		FParse::Param(FCommandLine::Get(), TEXT("CCLCampaignCapture")) && CapturedPhase != static_cast<int32>(State->GetPhase()))
	{
		CapturedPhase = static_cast<int32>(State->GetPhase());
		const FString Directory = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Tests/CampaignVisual"));
		IFileManager::Get().MakeDirectory(*Directory, true);
		Local->SetControlRotation(FRotator(-12.f, 0.f, 0.f));
		FScreenshotRequest::RequestScreenshot(Directory / FString::Printf(TEXT("phase-%d.png"), CapturedPhase), true, false);
	}
	if (GetWorld()->GetNetMode() != NM_Client)
	{
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			UCCLTestBridgeComponent::Ensure(It->Get());
		}
	}
	if (Role == TEXT("driver") && Local && Local->IsLocalController() && ASC && ASC->GetActivatableAbilities().Num() >= 4)
	{
		if (!bRegistered)
		{
			if (auto* Bridge = UCCLTestBridgeComponent::Find(Local))
			{
				Bridge->ServerCampaignTestReady();
				bRegistered = 1;
			}
		}
		if (bAttacking && Now >= NextInputAt)
		{
			Local->SetControlRotation(FRotator::ZeroRotator);
			ASC->AbilityInputTagPressed(CCLTags::Input_Attack);
			ASC->AbilityInputTagReleased(CCLTags::Input_Attack);
			NextInputAt = Now + 1.3;
		}
	}

	if (Local && Local->IsLocalController() && Pawn && State && State->GetPhase() == ECCLCampaignPhase::Victory && !bClientReported)
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("CCLProgressionSmoke")))
		{
			const auto* Player = Local->GetPlayerState<ACCLPlayerState>();
			if (!Player || !Check(Role == TEXT("driver") ? Player->GetInventory()->GetEntries().Num() == 2 : Player->GetInventory()->GetEntries().IsEmpty(),
				TEXT("inventory replication stays with owning player")))
			{
				return;
			}
		}
		if (State->GetRemainingGuards() != 0)
		{
			Check(false, TEXT("replicated victory has no remaining guards"));
			return;
		}
		if (!FParse::Param(FCommandLine::Get(), TEXT("CCLContentSmoke")) || CheckContentReplication(Local, Role))
		{
			UE_LOG(LogTemp, Display, TEXT("CCL_CAMPAIGN CLIENT PASS role=%s"), *Role);
			bClientReported = 1;
		}
	}

	if (bFailed || bComplete || GetWorld()->GetNetMode() == NM_Client || !Driver.IsValid() || !State || Now < NextStageAt)
	{
		return;
	}
	auto* Character = Cast<ACCLCharacter>(Driver->GetPawn());
	if (!Character)
	{
		return;
	}
	ACCLCampaignDirector* Director = nullptr;
	for (TActorIterator<ACCLCampaignDirector> It(GetWorld()); It; ++It)
	{
		Director = *It;
		break;
	}
	if (!Director)
	{
		Check(false, TEXT("director exists"));
		return;
	}

	if (Stage == 0)
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("CCLContentSmoke")) && !bContentReady)
		{
			bContentReady = TickContent(false);
			return;
		}
		if (FParse::Param(FCommandLine::Get(), TEXT("CCLProgressionSmoke")) && !bProgressionReady)
		{
			TickProgression();
			return;
		}
		if (!Check(State->GetPhase() == ECCLCampaignPhase::Village && State->GetRemainingGuards() == 2 &&
			Director->GetGuards().Num() == 2 && !Director->GetBoss(), TEXT("village starts with two guards and no boss")))
		{
			return;
		}
		Director->NotifyEnemyDefeated(Director->GetGuards()[0].Get());
		if (!Check(State->GetRemainingGuards() == 2, TEXT("living enemy notification rejected")))
		{
			return;
		}
		Target = Director->GetGuards()[0];
		NavigationStart = Target->GetActorLocation();
		Character->SetActorLocation(NavigationStart - FVector(450.f, 0.f, 0.f));
		NextStageAt = Now + 6.;
		Stage = 10;
		return;
	}
	if (Stage == 10)
	{
		const auto* AI = Target.IsValid() ? Cast<AAIController>(Target->GetController()) : nullptr;
		FNavLocation Projected;
		auto* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
		const bool bNavigation = Navigation && Navigation->ProjectPointToNavigation(NavigationStart, Projected);
		UE_LOG(LogTemp, Display, TEXT("CCL_CAMPAIGN navigation moved=%.1f health=%.1f brain=%d nav=%d"),
			Target.IsValid() ? FVector::Dist2D(Target->GetActorLocation(), NavigationStart) : -1.f,
			Character->GetAbilitySystemComponent()->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()),
			AI && AI->GetBrainComponent() && AI->GetBrainComponent()->IsRunning(), bNavigation);
		if (!Check(Target.IsValid() && FVector::Dist2D(Target->GetActorLocation(), NavigationStart) > 100.f &&
			Character->GetAbilitySystemComponent()->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) < 100.f,
			TEXT("live StateTree navigates and attacks on campaign map")))
		{
			return;
		}
		Character->GetAbilitySystemComponent()->SetNumericAttributeBase(UCCLHealthSet::GetHealthAttribute(), 100.f);
		PrepareEnemy(Target.Get());
		Stage = 1;
		return;
	}
	if (Stage == 1 || Stage == 2 || Stage == 3)
	{
		if (!Target.IsValid())
		{
			Check(false, TEXT("encounter target remains valid"));
			return;
		}
		if (!Target->IsDead())
		{
			return;
		}
		const int32 Remaining = State->GetRemainingGuards();
		Director->NotifyEnemyDefeated(Target.Get());
		if (!Check(State->GetRemainingGuards() == Remaining, TEXT("duplicate death ignored")))
		{
			return;
		}
		if (Stage == 1)
		{
			if (!Check(State->GetPhase() == ECCLCampaignPhase::Road && Remaining == 1 && !Director->GetBoss(), TEXT("first guard does not unlock boss")))
			{
				return;
			}
			PrepareEnemy(Director->GetGuards()[1].Get());
			Stage = 2;
		}
		else if (Stage == 2)
		{
			if (!Check(State->GetPhase() == ECCLCampaignPhase::Boss && Remaining == 0 && Director->GetBoss(), TEXT("all guards unlock boss")))
			{
				return;
			}

			if (FParse::Param(FCommandLine::Get(), TEXT("CCLContentSmoke")))
			{
				auto* Boss = Director->GetBoss();
				auto* Fighter = Boss->FindComponentByClass<UCCLFighterComponent>();
				Boss->GetAbilitySystemComponent()->CancelAllAbilities();
				Boss->SelectAttackPattern();
				if (!Check(Fighter->GetAttack() && Fighter->GetAttack()->Damage == 30.f && Fighter->GetAttack()->bGuardable, TEXT("boss starts with heavy attack")))
				{
					return;
				}

				Boss->GetAbilitySystemComponent()->SetNumericAttributeBase(UCCLHealthSet::GetHealthAttribute(), 120.f);
				Boss->SelectAttackPattern();
				if (!Check(Fighter->GetAttack() && !Fighter->GetAttack()->bGuardable && !Fighter->GetAttack()->bParryable && Fighter->GetAttack()->Radius == 100.f, TEXT("half-health boss selects unblockable wide sweep")))
				{
					return;
				}

				Character->GetAbilitySystemComponent()->CancelAllAbilities();
				Character->SetActorLocation(Boss->GetActorLocation() - FVector(110.f, 0.f, 0.f));
				Character->SetActorRotation(FRotator::ZeroRotator);
				auto* PlayerASC = CastChecked<UCCLAbilitySystemComponent>(Character->GetAbilitySystemComponent());
				PlayerASC->SetNumericAttributeBase(UCCLHealthSet::GetHealthAttribute(), 100.f);
				const auto Guard = PlayerASC->ApplyEffect(UCCLGuardEffect::StaticClass());
				auto* Combat = Boss->FindComponentByClass<UKSICombatComponent>();
				const uint32 AttackId = Combat->BeginAttack(Fighter->GetAttack(), -FVector::ForwardVector);
				FHitResult Hit(Character, nullptr, Character->GetActorLocation(), FVector::UpVector);
				Hit.ImpactPoint = Character->GetActorLocation();
				Combat->ResolveHit(Character, Hit, AttackId);
				Combat->EndAttack();
				PlayerASC->RemoveActiveGameplayEffect(Guard);
				if (!Check(PlayerASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) == 76.f, TEXT("boss sweep deals actual damage through frontal guard")))
				{
					return;
				}

				PlayerASC->SetNumericAttributeBase(UCCLHealthSet::GetHealthAttribute(), 100.f);
				Boss->SelectAttackPattern();
				if (!Check(Fighter->GetAttack()->bGuardable != 0, TEXT("enraged boss alternates sweep and heavy")))
				{
					return;
				}

				Boss->GetAbilitySystemComponent()->SetNumericAttributeBase(UCCLHealthSet::GetHealthAttribute(), 240.f);
			}
			PrepareEnemy(Director->GetBoss());
			Stage = 3;
		}
		else
		{
			if (!Check(State->GetPhase() == ECCLCampaignPhase::Victory, TEXT("boss defeat completes expedition")))
			{
				return;
			}
			UCCLTestBridgeComponent::Find(Driver.Get())->ClientCampaignTestStep(0);
			OldPawn = Character;
			Character->Die();
			UCCLTestBridgeComponent::Find(Driver.Get())->ClientCampaignTestStep(2);
			NextStageAt = Now + 2.;
			Stage = 4;
		}
		return;
	}
	if (Stage == 4 && Character != OldPawn.Get() && !Character->IsDead())
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("CCLContentSmoke")) && !TickContent(true))
		{
			return;
		}

		if (bProgressionReady && !CheckProgressionPersistence())
		{
			return;
		}
		if (!Check(State->GetPhase() == ECCLCampaignPhase::Victory, TEXT("victory survives individual respawn")))
		{
			return;
		}
		UE_LOG(LogTemp, Display, TEXT("CCL_CAMPAIGN SERVER PASS guards, boss, duplicate events and respawn"));
		bComplete = 1;
	}
}

void UCCLCampaignSmokeSubsystem::RegisterDriver(ACCLPlayerController* Controller)
{
	if (GetWorld()->GetNetMode() != NM_Client && !Driver.IsValid())
	{
		Driver = Controller;
		NextStageAt = GetWorld()->GetTimeSeconds() + (FParse::Param(FCommandLine::Get(), TEXT("CCLCampaignCapture")) ? 30. : 2.);
	}
}

void UCCLCampaignSmokeSubsystem::ExecuteClientStep(int32 Step)
{
	bAttacking = Step == 1;
	NextInputAt = GetWorld()->GetTimeSeconds() + 1.;
	if (Step == 2)
	{
		if (auto* Local = Cast<ACCLPlayerController>(GetWorld()->GetFirstPlayerController()))
		{
			Local->CCLRetry();
		}
	}
}

void UCCLCampaignSmokeSubsystem::PrepareEnemy(ACCLEnemyCharacter* Enemy)
{
	if (!Check(IsValid(Enemy), TEXT("next enemy exists")))
	{
		return;
	}
	for (TActorIterator<ACCLEnemyCharacter> It(GetWorld()); It; ++It)
	{
		if (auto* AI = Cast<AAIController>(It->GetController()))
		{
			AI->StopMovement();
			if (AI->GetBrainComponent())
			{
				AI->GetBrainComponent()->StopLogic(TEXT("Campaign progression test"));
			}
		}
		It->GetAbilitySystemComponent()->CancelAllAbilities();
	}
	Target = Enemy;
	auto* Character = Cast<ACCLCharacter>(Driver->GetPawn());
	Character->GetCharacterMovement()->StopMovementImmediately();
	Character->SetActorLocation(Enemy->GetActorLocation() - FVector(110.f, 0.f, 0.f));
	Character->SetActorRotation(FRotator::ZeroRotator);
	Character->GetAbilitySystemComponent()->SetNumericAttributeBase(UCCLStaminaSet::GetStaminaAttribute(), 100.f);
	UCCLTestBridgeComponent::Find(Driver.Get())->ClientCampaignTestStep(1);
	NextStageAt = GetWorld()->GetTimeSeconds() + 1.;
}

bool UCCLCampaignSmokeSubsystem::Check(bool bCondition, const TCHAR* Description)
{
	if (!bCondition)
	{
		bFailed = 1;
		UE_LOG(LogTemp, Error, TEXT("CCL_CAMPAIGN FAIL %s stage=%d"), Description, Stage);
		return false;
	}
	UE_LOG(LogTemp, Display, TEXT("CCL_CAMPAIGN CHECK %s"), Description);
	return true;
}

void UCCLCampaignSmokeSubsystem::ExecuteProgressionStep(int32 InStep, FGuid EntryId)
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("CCLProgressionSmoke")))
	{
		return;
	}
	auto* Local = Cast<ACCLPlayerController>(GetWorld()->GetFirstPlayerController());
	auto* State = Local ? Local->GetPlayerState<ACCLPlayerState>() : nullptr;
	if (!Local || !State)
	{
		return;
	}
	if (InStep == 0)
	{
		Local->ServerCollectNearby();
	}
	else if (InStep == 1)
	{
		Local->ServerEquipItem(EntryId);
	}
	else if (InStep == 2 || InStep == 4)
	{
		const int32 Index = InStep == 2 ? 0 : 1;
		if (State->GetLoadout()->GetSkills().IsValidIndex(Index))
		{
			Local->ServerLearnSkill(State->GetLoadout()->GetSkills()[Index]);
			if (InStep == 4)
			{
				Local->CloseInventory();
			}
		}
	}
	else if (InStep == 5)
	{
		Local->ServerUnequipToBag(EntryId, 15);
		Local->ServerMoveInventoryItem(EntryId, -1);
		Local->ServerMoveInventoryItem(EntryId, 16);
		Local->ServerMoveInventoryItem(FGuid::NewGuid(), 15);
		Local->ServerMoveInventoryItem(EntryId, 15);
	}
	else if (InStep == 6)
	{
		Local->ServerMoveInventoryItem(EntryId, 1);
	}
	else if (InStep == 7)
	{
		Local->HandPressed(CCLItemTags::Slot_LeftHand);
	}
	else if (InStep == 8)
	{
		Local->HandReleased(CCLItemTags::Slot_LeftHand);
	}
	else if (InStep == 9)
	{
		Local->ServerEquipToSlot(EntryId, CCLItemTags::Slot_RightHand);
	}
	else if (InStep == 10)
	{
		Local->HandPressed(CCLItemTags::Slot_RightHand);
	}
	else if (InStep == 11)
	{
		Local->HandReleased(CCLItemTags::Slot_RightHand);
	}
	else if (InStep == 3)
	{
		Local->ServerUseItem(EntryId);
		if (!Local->IsInventoryOpen())
		{
			Local->ToggleInventory();
		}
	}
}

void UCCLCampaignSmokeSubsystem::TickProgression()
{
	auto* State = Driver->GetPlayerState<ACCLPlayerState>();
	auto* Inventory = State ? State->GetInventory() : nullptr;
	auto* Loadout = State ? State->GetLoadout() : nullptr;
	auto* Character = Cast<ACCLCharacter>(Driver->GetPawn());
	auto* ASC = State ? State->GetCCLAbilitySystem() : nullptr;
	if (!Inventory || !Loadout || !Character || !ASC)
	{
		return;
	}
	auto Bonus = [ASC]() { return ASC->GetNumericAttribute(UCCLOffenseSet::GetAttackBonusAttribute()); };
	auto Collect = [this, Character](bool bEquipment)
	{
		const FName FixtureName = bEquipment ? TEXT("DA_IronGauntlets") : TEXT("DA_RecoveryPotion");
		for (TActorIterator<ACCLWorldPickup> It(GetWorld()); It; ++It)
		{
			if (It->Definition && It->Definition->GetFName() == FixtureName)
			{
				Character->SetActorLocation(It->GetActorLocation() + FVector(0.f, -80.f, 55.f));
				UCCLTestBridgeComponent::Find(Driver.Get())->ClientProgressionTestStep(0, FGuid());
				return true;
			}
		}
		return false;
	};
	switch (ProgressionStep)
	{
	case 0:
		if (!Check(Inventory->GetEntries().IsEmpty() && Loadout->GetPoints() == 1, TEXT("initial inventory and training points")) ||
			!Check(Collect(true), TEXT("equipment pickup exists")))
		{
			return;
		}
		break;
	case 1:
		if (!Check(Inventory->GetEntries().Num() == 1, TEXT("client collected equipment")))
		{
			return;
		}
		EquipmentId = Inventory->GetEntries()[0].Id;
		if (!Check(Collect(false), TEXT("recovery pickup exists")))
		{
			return;
		}
		break;
	case 2:
	{
		if (!Check(Inventory->GetEntries().Num() == 2, TEXT("client collected recovery stack")))
		{
			return;
		}
		const auto* Potions = Inventory->GetEntries().FindByPredicate([this](const FKSIInventoryEntry& Entry) { return Entry.Id != EquipmentId; });
		if (!Check(Potions && Potions->Definition && Potions->Definition->GetFName() == TEXT("DA_RecoveryPotion") &&
			Potions->Quantity == 3, TEXT("server recovery pickup identity and quantity retained")))
		{
			return;
		}
		PotionId = Potions->Id;
		UKSIItemDefinition* Definition = Potions->Definition;
		if (!Check(!Inventory->Add(Definition, -1).IsValid() && !Inventory->Remove(PotionId, -1) &&
			!Inventory->Remove(FGuid::NewGuid(), 1), TEXT("invalid inventory mutations rejected")) ||
			!Check(!Loadout->Use(PotionId) && Inventory->Find(PotionId)->Quantity == 3, TEXT("full health does not consume recovery item")) ||
			!Check(FMath::IsNearlyEqual(ProbeDamage(), 20.f), TEXT("base damage before equipment")))
		{
			return;
		}
		AActor* Container = GetWorld()->SpawnActor<AActor>();
		auto* Storage = NewObject<UCCLInventoryComponent>(Container);
		Storage->Capacity = 1;
		Storage->RegisterComponent();
		const FGuid Stack = Storage->Add(Definition, 20);
		const bool bStorageValid = Stack.IsValid() && !Storage->Add(Definition, 1).IsValid() &&
			!Storage->Remove(Stack, 21) && Storage->Remove(Stack, 20) && Storage->GetEntries().IsEmpty();
		Container->Destroy();
		if (!Check(bStorageValid, TEXT("generic actor inventory enforces stack and slot limits")))
		{
			return;
		}
		UCCLTestBridgeComponent::Find(Driver.Get())->ClientProgressionTestStep(1, EquipmentId);
		break;
	}
	case 3:
		if (!Check(Loadout->GetEquippedId() == EquipmentId && FMath::IsNearlyEqual(Bonus(), 10.f), TEXT("client equipment applies GAS bonus")) ||
			!Check(Loadout->Equip(EquipmentId) && FMath::IsNearlyEqual(Bonus(), 10.f), TEXT("repeated equip does not stack effects")) ||
			!Check(!Loadout->Equip(FGuid::NewGuid()) && Loadout->GetEquippedId() == EquipmentId, TEXT("unowned equipment rejected")))
		{
			return;
		}
		ASC->SetNumericAttributeBase(UCCLHealthSet::GetHealthAttribute(), 40.f);
		UCCLTestBridgeComponent::Find(Driver.Get())->ClientProgressionTestStep(3, PotionId);
		break;
	case 4:
		if (!Check(ASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) == 90.f && Inventory->Find(PotionId)->Quantity == 2,
			TEXT("client recovery restores health and consumes once")))
		{
			return;
		}
		UCCLTestBridgeComponent::Find(Driver.Get())->ClientProgressionTestStep(2, FGuid());
		break;
	case 5:
		if (!Check(Loadout->GetSkills().Num() == 2 && Loadout->IsLearned(Loadout->GetSkills()[0]) && Loadout->GetPoints() == 0 &&
			FMath::IsNearlyEqual(Bonus(), 15.f), TEXT("client training spends one point and grants bonus")) ||
			!Check(!Loadout->Learn(Loadout->GetSkills()[0]) && !Loadout->Learn(Loadout->GetSkills()[1]) && Loadout->GetPoints() == 0,
			TEXT("duplicate training and insufficient points rejected")) ||
			!Check(FMath::IsNearlyEqual(ProbeDamage(), 35.f), TEXT("equipment and training change actual damage")))
		{
			return;
		}
		if (!Check(Loadout->Equip(FGuid()) && FMath::IsNearlyEqual(Bonus(), 5.f) && Loadout->Equip(EquipmentId) && FMath::IsNearlyEqual(Bonus(), 15.f),
			TEXT("unequip removes only equipment contribution")))
		{
			return;
		}
		Loadout->GrantPoints(1);
		UCCLTestBridgeComponent::Find(Driver.Get())->ClientProgressionTestStep(4, FGuid());
		break;
	case 6:
		if (!Check(Loadout->IsLearned(Loadout->GetSkills()[1]) && Loadout->GetPoints() == 0 &&
			ASC->GetNumericAttribute(UCCLHealthSet::GetMaxHealthAttribute()) == 125.f, TEXT("vitality training increases GAS maximum")))
		{
			return;
		}
		UCCLTestBridgeComponent::Find(Driver.Get())->ClientProgressionTestStep(5, EquipmentId);
		break;
	case 7:
		if (!Check(Inventory->Find(EquipmentId)->Slot == 15 && Inventory->Find(PotionId)->Slot == 1 && Inventory->GetEntries().Num() == 2,
		           TEXT("remote move to empty slot preserves inventory")))
		{
			return;
		}

		UCCLTestBridgeComponent::Find(Driver.Get())->ClientProgressionTestStep(6, EquipmentId);
		break;
	case 8:
		if (!Check(Inventory->Find(EquipmentId)->Slot == 1 && Inventory->Find(PotionId)->Slot == 15 && !Loadout->IsEquipped(EquipmentId) &&
		               FMath::IsNearlyEqual(Bonus(), 5.f),
		           TEXT("remote unequip and occupied slot swap preserves ownership")))
		{
			return;
		}

		if (!Check(Loadout->Equip(EquipmentId), TEXT("reequip after remote bag move")))
		{
			return;
		}

		TestShieldId = Inventory->Add(FCCLSessionCodec::Item(TEXT("DA_TrainingShield")), 1);
		if (!Check(Loadout->Equip(TestShieldId, CCLItemTags::Slot_LeftHand), TEXT("remote hand fixture equipped")))
		{
			return;
		}

		break;
	case 9:
		UCCLTestBridgeComponent::Find(Driver.Get())->ClientProgressionTestStep(7, FGuid());
		break;
	case 10:
		if (!Check(ASC->HasMatchingGameplayTag(CCLTags::State_Guard), TEXT("remote left-hand shield activates predicted guard on server")))
		{
			return;
		}

		UCCLTestBridgeComponent::Find(Driver.Get())->ClientProgressionTestStep(8, FGuid());
		break;
	case 11:
		if (!Check(!ASC->HasMatchingGameplayTag(CCLTags::State_Guard), TEXT("remote left-hand release ends guard")))
		{
			return;
		}

		Inventory->Remove(TestShieldId, 1);
		TestStaffId = Inventory->Add(FCCLSessionCodec::Item(TEXT("DA_TrainingStaff")), 1);
		UCCLTestBridgeComponent::Find(Driver.Get())->ClientProgressionTestStep(9, TestStaffId);
		break;
	case 12:
		if (!Check(Loadout->GetEquippedId() == TestStaffId && Loadout->GetEquippedId(CCLItemTags::Slot_LeftHand) == TestStaffId &&
		               Inventory->Find(EquipmentId)->Slot >= 0,
		           TEXT("remote two-hand equip returns previous weapon")))
		{
			return;
		}

		UCCLTestBridgeComponent::Find(Driver.Get())->ClientProgressionTestStep(10, FGuid());
		break;
	case 13:
		if (!Check(ASC->HasMatchingGameplayTag(CCLTags::State_Guard), TEXT("remote two-hand secondary action guards")))
		{
			return;
		}

		UCCLTestBridgeComponent::Find(Driver.Get())->ClientProgressionTestStep(11, FGuid());
		break;
	case 14:
		if (!Check(!ASC->HasMatchingGameplayTag(CCLTags::State_Guard) && Inventory->Remove(TestStaffId, 1) && Loadout->Equip(EquipmentId),
		           TEXT("remote secondary release and gear fixture cleanup")))
		{
			return;
		}

		ASC->SetNumericAttributeBase(UCCLHealthSet::GetHealthAttribute(), 100.f);
		bProgressionReady = 1;
		UE_LOG(LogTemp, Display, TEXT("CCL_PROGRESSION ACTIONS PASS"));
		return;
	}
	++ProgressionStep;
	NextStageAt = GetWorld()->GetTimeSeconds() + 1.5;
}

bool UCCLCampaignSmokeSubsystem::CheckProgressionPersistence()
{
	const auto* State = Driver->GetPlayerState<ACCLPlayerState>();
	const auto* Inventory = State->GetInventory();
	const auto* Loadout = State->GetLoadout();
	const auto* ASC = State->GetCCLAbilitySystem();
	const bool bValid = Inventory->GetEntries().Num() == 2 && Inventory->Find(PotionId) && Inventory->Find(PotionId)->Quantity == 2 &&
		Loadout->GetEquippedId() == EquipmentId && Loadout->IsLearned(Loadout->GetSkills()[0]) && Loadout->IsLearned(Loadout->GetSkills()[1]) &&
		FMath::IsNearlyEqual(ASC->GetNumericAttribute(UCCLOffenseSet::GetAttackBonusAttribute()), 15.f) &&
		ASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) == 125.f && Loadout->GetPoints() == 0;
	if (!Check(bValid, TEXT("inventory equipment training and maximum health survive respawn")))
	{
		return false;
	}
	UE_LOG(LogTemp, Display, TEXT("CCL_PROGRESSION PERSISTENCE PASS"));
	return true;
}

float UCCLCampaignSmokeSubsystem::ProbeDamage()
{
	AActor* Source = Driver->GetPawn();
	auto* Combat = Source->FindComponentByClass<UKSICombatComponent>();
	auto* Fighter = Source->FindComponentByClass<UCCLFighterComponent>();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* Dummy = GetWorld()->SpawnActor<ACCLHealthTarget>(Source->GetActorLocation() + FVector(110.f, 0.f, 0.f), FRotator::ZeroRotator, Params);
	if (!Dummy)
	{
		return -1.f;
	}
	const float Before = Dummy->GetAbilitySystemComponent()->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute());
	const uint32 Id = Combat->BeginAttack(Fighter->GetAttack(), FVector::ForwardVector);
	FHitResult Hit(Dummy, nullptr, Dummy->GetActorLocation(), FVector::UpVector);
	Hit.ImpactPoint = Dummy->GetActorLocation();
	Combat->ResolveHit(Dummy, Hit, Id);
	Combat->EndAttack();
	const float Damage = Before - Dummy->GetAbilitySystemComponent()->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute());
	Dummy->Destroy();
	return Damage;
}

bool UCCLCampaignSmokeSubsystem::TickContent(bool bAfterVictory)
{
	auto* Player = Driver->GetPlayerState<ACCLPlayerState>();
	APawn* Pawn = Driver->GetPawn();
	if (!Player || !Pawn)
	{
		return false;
	}

	auto* Content = Player->GetExpedition();
	auto* Inventory = Player->GetInventory();
	ACCLVillageSteward* NPC = nullptr;
	for (TActorIterator<ACCLVillageSteward> It(GetWorld()); It; ++It)
	{
		if (!It->IsA<ACCLLifeVillager>())
		{
			NPC = *It;
			break;
		}
	}

	if (!Check(NPC != nullptr, TEXT("village steward exists")))
	{
		return false;
	}

	if (!bAfterVictory)
	{
		switch (ContentStep)
		{
		case 0:
		{
			Pawn->SetActorLocation(NPC->GetActorLocation() + FVector(800.f, 0.f, 0.f));
			if (!Check(!Content->Talk(NPC) && !Content->Buy(NPC) && Content->GetCoins() == 30, TEXT("distant NPC requests rejected")))
			{
				return false;
			}

			Pawn->SetActorLocation(NPC->GetActorLocation() + FVector(0.f, -150.f, 0.f));
			for (const FGameplayTag StateTag : {FGameplayTag(CCLTags::State_Busy), FGameplayTag(CCLTags::State_Stagger)})
			{
				auto* PlayerASC = Player->GetAbilitySystemComponent();
				PlayerASC->AddLooseGameplayTag(StateTag);
				const bool bRejected = !Content->Talk(NPC) && !Content->Buy(NPC) && Content->GetCoins() == 30 &&
					Content->GetQuest() == ECCLQuestStatus::Available && Inventory->GetEntries().IsEmpty();
				PlayerASC->RemoveLooseGameplayTag(StateTag);
				if (!Check(bRejected, TEXT("busy and staggered NPC requests preserve personal state")))
				{
					return false;
				}
			}

			NPC->Offer.InitialStock = 3;
			const int32 Capacity = Inventory->Capacity;
			Inventory->Capacity = 1;
			auto* Item = LoadObject<UKSIItemDefinition>(nullptr, TEXT("/Game/Progression/DA_IronGauntlets.DA_IronGauntlets"));
			const FGuid Id = Inventory->Add(Item, 1);
			if (!Check(Id.IsValid() && !Content->Buy(NPC) && Content->GetCoins() == 30, TEXT("full inventory purchase does not debit")))
			{
				return false;
			}

			Inventory->Remove(Id, 1);
			Inventory->Capacity = Capacity;
			UCCLTestBridgeComponent::Find(Driver.Get())->ClientContentTestStep(0);
			break;
		}
		case 1:
			if (!Check(Content->GetQuest() == ECCLQuestStatus::Accepted && !Content->Talk(NPC) && Player->GetLoadout()->GetPoints() == 1, TEXT("client accepts quest; premature reward rejected")))
			{
				return false;
			}

			UCCLTestBridgeComponent::Find(Driver.Get())->ClientContentTestStep(1);
			UCCLTestBridgeComponent::Find(Driver.Get())->ClientContentTestStep(1);
			UCCLTestBridgeComponent::Find(Driver.Get())->ClientContentTestStep(1);
			UCCLTestBridgeComponent::Find(Driver.Get())->ClientContentTestStep(1);
			break;
		case 2:
			if (!Check(Content->GetCoins() == 0 && Inventory->GetEntries().Num() == 1 && Inventory->GetEntries()[0].Quantity == 3,
				TEXT("client purchases three; fourth rejected for insufficient funds")))
			{
				return false;
			}

			for (TActorIterator<ACCLEnemyCharacter> It(GetWorld()); It; ++It)
			{
				const auto* Definition = It->GetDefinition();
				if (Definition && Definition->GetFName() == TEXT("DA_RoadRaider") && !Check(It->FindComponentByClass<UCCLFighterComponent>()->GetAttack()->Damage == 12.f &&
					It->GetCharacterMovement()->MaxWalkSpeed == 330.f, TEXT("raider has distinct attack and approach speed")))
				{
					return false;
				}
			}

			if (FParse::Param(FCommandLine::Get(), TEXT("CCLCampaignCapture")) && Driver->IsLocalController())
			{
				Driver->SetControlRotation(FRotator(-8.f, 90.f, 0.f));
				FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Tests/CampaignVisual/steward.png")), true, false);
			}
			NextStageAt = GetWorld()->GetTimeSeconds() + 2.;
			UE_LOG(LogTemp, Display, TEXT("CCL_CONTENT SERVICES PASS"));
			ContentStep = 10;
			return true;
		}
	}
	else
	{
		if (ContentStep == 10)
		{
			if (!Check(Content->GetQuest() == ECCLQuestStatus::Accepted && Content->GetCoins() == 0 && Inventory->GetEntries()[0].Quantity == 3,
				TEXT("quest and purchases survive individual respawn")))
			{
				return false;
			}

			Pawn->SetActorLocation(NPC->GetActorLocation() + FVector(0.f, -150.f, 0.f));
			UCCLTestBridgeComponent::Find(Driver.Get())->ClientContentTestStep(0);
		}
		else if (ContentStep == 11)
		{
			if (!Check(Content->GetQuest() == ECCLQuestStatus::Rewarded && Content->GetCoins() == 60 && Player->GetLoadout()->GetPoints() == 2,
				TEXT("client returns for quest coins and training reward")) || !Check(!Content->Talk(NPC) && Content->GetCoins() == 60 && Player->GetLoadout()->GetPoints() == 2,
				TEXT("duplicate reward rejected")))
			{
				return false;
			}

			if (!Check(!Content->Buy(NPC) && Content->GetCoins() == 60 && Inventory->GetEntries().Num() == 1 &&
				Inventory->GetEntries()[0].Quantity == 3, TEXT("sold-out purchase preserves coins and items")))
			{
				return false;
			}

			NPC->Offer.InitialStock = 23;
			if (!Check(!Content->Buy(NPC) && Content->GetCoins() == 60 && Inventory->GetEntries()[0].Quantity == 3,
				TEXT("initial stock changes never refill an existing resource")))
			{
				return false;
			}

			// Deliberately differ from class defaults so remote and late clients must receive the offer.
			NPC->Offer.Item = TSoftObjectPtr<UKSIItemDefinition>(FSoftObjectPath(TEXT("/Game/Progression/DA_IronGauntlets.DA_IronGauntlets")));
			NPC->Offer.Price = 17;
			NPC->ForceNetUpdate();
			UE_LOG(LogTemp, Display, TEXT("CCL_CONTENT REWARD PASS"));
			return true;
		}
	}
	++ContentStep;
	NextStageAt = GetWorld()->GetTimeSeconds() + 1.5;
	return false;
}


bool UCCLCampaignSmokeSubsystem::CheckContentReplication(ACCLPlayerController* Local, const FString& Role)
{
	if (!ContentReplicationStarted)
	{
		ContentReplicationStarted = FPlatformTime::Seconds();
	}

	const auto* Player = Local->GetPlayerState<ACCLPlayerState>();
	const auto* Content = Player ? Player->GetExpedition() : nullptr;
	const auto* Inventory = Player ? Player->GetInventory() : nullptr;
	const auto* Loadout = Player ? Player->GetLoadout() : nullptr;
	const bool bDriver = Role == TEXT("driver");
	const bool bPersonalReady = Content && Inventory && Loadout &&
		Content->GetQuest() == (bDriver ? ECCLQuestStatus::Rewarded : ECCLQuestStatus::Available) &&
		Content->GetCoins() == (bDriver ? 60 : 30) && Loadout->GetPoints() == (bDriver ? 2 : 1) &&
		(bDriver ? Inventory->GetEntries().Num() == 1 && Inventory->GetEntries()[0].Quantity == 3 : Inventory->GetEntries().IsEmpty());
	bool bOfferReady = false;
	for (TActorIterator<ACCLVillageSteward> It(GetWorld()); It; ++It)
	{
		if (!It->IsA<ACCLLifeVillager>())
		{
			const auto& Offer = It->Offer;
			bOfferReady = Offer.Price == 17 && Offer.InitialStock == 23 &&
				Offer.Item.ToSoftObjectPath() == FSoftObjectPath(TEXT("/Game/Progression/DA_IronGauntlets.DA_IronGauntlets")) &&
				Offer.StockResource.GetTagName() == TEXT("Agent.Resource.RecoveryPotion");
			break;
		}
	}

	if (bPersonalReady && bOfferReady)
	{
		UE_LOG(LogTemp, Display, TEXT("CCL_CONTENT CLIENT PASS role=%s offer=17 stockSetting=23 personal=1"), *Role);
		return true;
	}

	if (FPlatformTime::Seconds() - ContentReplicationStarted > 20.)
	{
		UE_LOG(LogTemp, Display, TEXT("CCL_CONTENT replication personal=%d offer=%d role=%s"), bPersonalReady, bOfferReady, *Role);
		Check(false, TEXT("content offer and owner state replication timeout"));
	}
	return false;
}
