#include "CCLWeaponAbility.h"

#include "CCLActionComponent.h"
#include "AbilitySystem/CCLGameplayTags.h"
#include "Combat/CCLProjectile.h"
#include "Combat/CCLCombatDefinition.h"
#include "Items/CCLItemDefinition.h"
#include "Items/CCLInventoryComponent.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

namespace CCLActionTags
{
UE_DEFINE_GAMEPLAY_TAG(Fire, "Action.Weapon.Fire");
UE_DEFINE_GAMEPLAY_TAG(Reload, "Action.Weapon.Reload");
UE_DEFINE_GAMEPLAY_TAG(Aim, "Action.Weapon.Aim");
UE_DEFINE_GAMEPLAY_TAG(Aiming, "State.Aiming");
}

namespace
{
const UCCLProjectileProfile* Profile(const UObject* Source)
{
	if (const auto* Intrinsic = Cast<UCCLProjectileProfile>(Source))
	{
		return Intrinsic;
	}

	const auto* Item = Cast<UCCLItemDefinition>(Source);
	const auto* Fragment = Item ? Item->FindFragment<FCCLItemFragment_ProjectileWeapon>() : nullptr;
	return Fragment ? Fragment->Profile : nullptr;
}

FGuid SourceId(const FGameplayAbilityActorInfo* Info, FGameplayAbilitySpecHandle Handle)
{
	const auto* Actions = Info && Info->OwnerActor.IsValid() ? Info->OwnerActor->FindComponentByClass<UCCLActionComponent>() : nullptr;
	const auto* Source = Actions ? Actions->FindSource(Handle) : nullptr;
	return Source ? Source->Id : FGuid();
}
}

UCCLProjectileAbility::UCCLProjectileAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ActivationBlockedTags.AddTag(CCLTags::State_Dead);
	ActivationBlockedTags.AddTag(CCLTags::State_Busy);
	ActivationBlockedTags.AddTag(CCLTags::State_Stagger);
	ActivationOwnedTags.AddTag(CCLTags::State_Busy);
}

void UCCLProjectileAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
	FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* EventData)
{
	const auto* Settings = Profile(GetCurrentSourceObject());
	auto* Avatar = Info->AvatarActor.Get();
	auto* Inventory = Info->OwnerActor->FindComponentByClass<UCCLInventoryComponent>();
	const FGuid Id = SourceId(Info, Handle);
	if (!Avatar || !Settings || !Settings->Combat || !FMath::IsFinite(Settings->Speed) || Settings->Speed <= 0 ||
		!FMath::IsFinite(Settings->Gravity) || !FMath::IsFinite(Settings->FireInterval) || Settings->FireInterval < 0.05f ||
		(Settings->MagazineSize > 0 && (!Inventory || !Inventory->CanFire(Id))))
	{
		EndAbility(Handle, Info, ActivationInfo, true, true);
		return;
	}

	FVector Eye;
	FRotator Rotation;
	Avatar->GetActorEyesViewPoint(Eye, Rotation);
	FHitResult Sight;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(CCLProjectileAim), false, Avatar);
	const FVector Far = Eye + Rotation.Vector() * 20000;
	GetWorld()->LineTraceSingleByChannel(Sight, Eye, Far, ECC_Visibility, Query);
	const FVector Aim = Sight.bBlockingHit ? Sight.ImpactPoint : Far;
	const FVector Muzzle = Avatar->GetActorLocation() + FVector(0, 0, 45) + Rotation.Vector() * 55;
	FHitResult Obstruction;
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Obstruction, Eye, Muzzle, ECC_Visibility, Query);
	if (bBlocked)
	{
		EndAbility(Handle, Info, ActivationInfo, true, true);
		return;
	}

	const FVector Direction = (Aim - Muzzle).GetSafeNormal();
	const FTransform Transform(Direction.Rotation(), Muzzle);
	const TSubclassOf<ACCLProjectile> Class = Settings->ProjectileClass ? Settings->ProjectileClass.Get() : ACCLProjectile::StaticClass();
	auto* Shot = GetWorld()->SpawnActorDeferred<ACCLProjectile>(Class, Transform, Avatar, Cast<APawn>(Avatar),
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Shot || (Settings->MagazineSize > 0 && !Inventory->ConsumeShot(Id)))
	{
		if (Shot)
		{
			Shot->Destroy();
		}

		EndAbility(Handle, Info, ActivationInfo, true, true);
		return;
	}

	Shot->Launch(Info->AbilitySystemComponent.Get(), Settings->Combat, Direction * Settings->Speed, Settings->Gravity);
	Shot->FinishSpawning(Transform);
	auto* Delay = UAbilityTask_WaitDelay::WaitDelay(this, Settings->FireInterval);
	Delay->OnFinish.AddDynamic(this, &ThisClass::Finish);
	Delay->ReadyForActivation();
}

void UCCLProjectileAbility::Finish()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

UCCLReloadAbility::UCCLReloadAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ActivationBlockedTags.AddTag(CCLTags::State_Dead);
	ActivationBlockedTags.AddTag(CCLTags::State_Busy);
	ActivationBlockedTags.AddTag(CCLTags::State_Stagger);
	ActivationOwnedTags.AddTag(CCLTags::State_Busy);
}

void UCCLReloadAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
	FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* EventData)
{
	const auto* Settings = Profile(GetCurrentSourceObject());
	auto* Inventory = Info->OwnerActor->FindComponentByClass<UCCLInventoryComponent>();
	if (!Settings || Settings->MagazineSize <= 0 || !Inventory || !Inventory->CanReload(SourceId(Info, Handle)) ||
		!FMath::IsFinite(Settings->ReloadSeconds) || Settings->ReloadSeconds < 0.05f)
	{
		EndAbility(Handle, Info, ActivationInfo, true, true);
		return;
	}

	auto* Delay = UAbilityTask_WaitDelay::WaitDelay(this, Settings->ReloadSeconds);
	Delay->OnFinish.AddDynamic(this, &ThisClass::CompleteReload);
	Delay->ReadyForActivation();
}

void UCCLReloadAbility::CompleteReload()
{
	auto* Inventory = CurrentActorInfo->OwnerActor.IsValid() ?
		CurrentActorInfo->OwnerActor->FindComponentByClass<UCCLInventoryComponent>() : nullptr;
	const bool bSucceeded = Inventory && Inventory->Reload(SourceId(CurrentActorInfo, CurrentSpecHandle));
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, !bSucceeded);
}

UCCLAimAbility::UCCLAimAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ActivationBlockedTags.AddTag(CCLTags::State_Dead);
	ActivationBlockedTags.AddTag(CCLTags::State_Stagger);
	ActivationOwnedTags.AddTag(CCLActionTags::Aiming);
}

void UCCLAimAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
	FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* EventData)
{
	// The source-specific release request cancels this ability through GAS.
}
