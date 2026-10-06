#include "CCLPlayerState.h"

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
	Inventory = CreateDefaultSubobject<UCCLInventoryComponent>(TEXT("Inventory"));
	Loadout = CreateDefaultSubobject<UCCLLoadoutComponent>(TEXT("Loadout"));
}

UAbilitySystemComponent* ACCLPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystem;
}
