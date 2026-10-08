#include "CCLPlayerState.h"
#include "Campaign/CCLExpeditionComponent.h"
#include "Actions/CCLActionComponent.h"
#include "Agents/CCLAccountComponent.h"

#include "AbilitySystem/CCLAbilitySystemComponent.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "AbilitySystem/CCLStaminaSet.h"
#include "AbilitySystem/CCLOffenseSet.h"
#include "Items/CCLInventoryComponent.h"
#include "Items/CCLLoadoutComponent.h"

ACCLPlayerState::ACCLPlayerState()
{
	SetNetUpdateFrequency(30.f);
	AbilitySystem = CreateDefaultSubobject<UCCLAbilitySystemComponent>(TEXT("AbilitySystem"));
	Health = CreateDefaultSubobject<UCCLHealthSet>(TEXT("Health"));
	Stamina = CreateDefaultSubobject<UCCLStaminaSet>(TEXT("Stamina"));
	Offense = CreateDefaultSubobject<UCCLOffenseSet>(TEXT("Offense"));
	Expedition = CreateDefaultSubobject<UCCLExpeditionComponent>(TEXT("Expedition"));
	Inventory = CreateDefaultSubobject<UCCLInventoryComponent>(TEXT("Inventory"));
	Loadout = CreateDefaultSubobject<UCCLLoadoutComponent>(TEXT("Loadout"));
	Actions = CreateDefaultSubobject<UCCLActionComponent>(TEXT("Actions"));
	Account = CreateDefaultSubobject<UCCLAccountComponent>(TEXT("Account"));
}

UAbilitySystemComponent* ACCLPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystem;
}
