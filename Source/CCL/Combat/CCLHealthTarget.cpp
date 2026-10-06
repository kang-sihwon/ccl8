#include "CCLHealthTarget.h"

#include "AbilitySystem/CCLAbilitySystemComponent.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ACCLHealthTarget::ACCLHealthTarget()
{
	bReplicates = true;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));

	if (Cube.Succeeded())
	{
		Mesh->SetStaticMesh(Cube.Object);
	}

	AbilitySystem = CreateDefaultSubobject<UCCLAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	Health = CreateDefaultSubobject<UCCLHealthSet>(TEXT("Health"));
}

void ACCLHealthTarget::BeginPlay()
{
	Super::BeginPlay();
	AbilitySystem->InitAbilityActorInfo(this, this);
	AbilitySystem->GetGameplayAttributeValueChangeDelegate(UCCLHealthSet::GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
}

UAbilitySystemComponent* ACCLHealthTarget::GetAbilitySystemComponent() const
{
	return AbilitySystem;
}

void ACCLHealthTarget::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (HasAuthority() && Data.NewValue <= 0.f)
	{
		Destroy();
	}
}
