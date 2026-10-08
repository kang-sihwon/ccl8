#include "CCLProjectileSmokeSubsystem.h"

#include "Actions/CCLActionComponent.h"
#include "Actions/CCLWeaponAbility.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "AbilitySystem/CCLGameplayTags.h"
#include "Combat/CCLHealthTarget.h"
#include "Combat/CCLProjectile.h"
#include "Items/CCLInventoryComponent.h"
#include "Items/CCLItemDefinition.h"
#include "Session/CCLSessionRecord.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

bool UCCLProjectileSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	const auto* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && FParse::Param(FCommandLine::Get(), TEXT("CCLProjectileSmoke"));
#endif
}

TStatId UCCLProjectileSmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLProjectileSmokeSubsystem, STATGROUP_Tickables);
}

void UCCLProjectileSmokeSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bFinished || !GetWorld()->HasBegunPlay() || GetWorld()->GetNetMode() == NM_Client)
	{
		return;
	}

	Elapsed += DeltaTime;
	if (!bStarted)
	{
		bStarted = 1;
		Source = GetWorld()->SpawnActor<ACCLHealthTarget>(FVector(10000, 10000, 5000), FRotator::ZeroRotator);
		Target = GetWorld()->SpawnActor<ACCLHealthTarget>(FVector(10800, 10000, 5000), FRotator::ZeroRotator);
		Check(Source && Target, TEXT("health-only source and target spawn"));
		if (!Source || !Target)
		{
			bFinished = 1;
			return;
		}

		auto* Inventory = NewObject<UCCLInventoryComponent>(Source);
		Source->AddInstanceComponent(Inventory);
		Inventory->RegisterComponent();
		auto* Rifle = FCCLSessionCodec::Item(TEXT("DA_Rifle"));
		auto* Bullets = FCCLSessionCodec::Item(TEXT("DA_Bullets"));
		const FGuid RifleId = Inventory->Add(Rifle, 1);
		Inventory->Add(Bullets, 10);
		Check(RifleId.IsValid() && !Inventory->CanFire(RifleId), TEXT("new weapon begins empty"));
		Check(Inventory->Reload(RifleId), TEXT("reserve ammo transfers into magazine"));
		Check(Inventory->Find(RifleId) && Inventory->Find(RifleId)->LoadedAmmo == 1, TEXT("rifle magazine capacity enforced"));
		Check(!Inventory->Reload(RifleId), TEXT("full magazine does not consume ammunition"));
		Check(Inventory->ConsumeShot(RifleId) && !Inventory->ConsumeShot(RifleId), TEXT("one round supports exactly one shot"));
		Check(Inventory->Reload(RifleId), TEXT("second reload uses remaining reserves"));
		FCCLSessionRecord Record;
		for (const auto& Entry : Inventory->GetEntries())
		{
			auto& Saved = Record.Items.AddDefaulted_GetRef();
			Saved.Id = Entry.Id;
			Saved.Definition = Entry.Definition->GetFName();
			Saved.Quantity = Entry.Quantity;
			Saved.Slot = Entry.Slot;
			Saved.LoadedAmmo = Entry.LoadedAmmo;
		}

		TArray<uint8> Bytes;
		FCCLSessionRecord Restored;
		Check(FCCLSessionCodec::Encode(Record, Bytes) && FCCLSessionCodec::Decode(Bytes, Restored) &&
			Restored.Items[0].LoadedAmmo == 1, TEXT("item-specific ammo survives session codec"));
		auto* Actions = NewObject<UCCLActionComponent>(Source);
		Source->AddInstanceComponent(Actions);
		Actions->RegisterComponent();
		auto* Base = LoadObject<UCCLProjectileProfile>(nullptr, TEXT("/Game/Progression/DA_PistolBallistics.DA_PistolBallistics"));
		Check(Base != nullptr, TEXT("ballistic profile loaded"));
		if (!Base)
		{
			bFinished = 1;
			return;
		}

		auto* Intrinsic = DuplicateObject<UCCLProjectileProfile>(Base, Actions);
		Intrinsic->MagazineSize = 0;
		Intrinsic->Gravity = 0;
		FCCLActionGrant Grant;
		Grant.Action = CCLActionTags::Fire;
		Grant.Ability = UCCLProjectileAbility::StaticClass();
		const FGuid IntrinsicId(1, 4, 5, 6);
		Check(Actions->RegisterSource(IntrinsicId, Intrinsic, {Grant}) && Actions->Execute(IntrinsicId, Grant.Action),
			TEXT("intrinsic source fires through shared GAS action without equipment or stamina"));
		Source->GetAbilitySystemComponent()->AddLooseGameplayTag(CCLTags::State_Dead);
		Source->Destroy();
		return;
	}

	if (Elapsed > 2)
	{
		Check(IsValid(Target) && FMath::IsNearlyEqual(Target->GetAbilitySystemComponent()->GetNumericAttribute(
			UCCLHealthSet::GetHealthAttribute()), 80.f), TEXT("moving projectile retains its effect after source Actor is destroyed"));
		int32 Count = 0;
		for (TActorIterator<ACCLProjectile> It(GetWorld()); It; ++It)
		{
			++Count;
		}

		Check(Count == 0, TEXT("impact retires projectile exactly once"));
		bFinished = 1;
		UE_LOG(LogTemp, Display, TEXT("CCL_PROJECTILE %s"), bFailed ? TEXT("FAIL") : TEXT("PASS"));
		GetWorld()->GetFirstPlayerController()->ConsoleCommand(TEXT("quit"));
	}
}

void UCCLProjectileSmokeSubsystem::Check(bool bPassed, const TCHAR* Message)
{
	if (!bPassed)
	{
		bFailed = 1;
	}

	UE_LOG(LogTemp, Display, TEXT("CCL_PROJECTILE %s %s"), bPassed ? TEXT("CHECK") : TEXT("FAIL"), Message);
}
