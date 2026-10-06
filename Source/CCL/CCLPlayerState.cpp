#include "CCLPlayerState.h"

#include "AbilitySystem/CCLAbilitySystemComponent.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "AbilitySystem/CCLStaminaSet.h"

ACCLPlayerState::ACCLPlayerState()
{
	SetNetUpdateFrequency(30.f);
	AbilitySystem = CreateDefaultSubobject<UCCLAbilitySystemComponent>(TEXT("AbilitySystem"));
	Health = CreateDefaultSubobject<UCCLHealthSet>(TEXT("Health"));
	Stamina = CreateDefaultSubobject<UCCLStaminaSet>(TEXT("Stamina"));
}

UAbilitySystemComponent* ACCLPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystem;
}
