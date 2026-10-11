#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CCLEnemyDefinition.generated.h"

class UCCLCombatDefinition;

USTRUCT(BlueprintType)
struct CCL_API FCCLEnemyAttackPattern
{
	GENERATED_BODY()

	// Empty uses the attack of the fighter's current weapon.
	UPROPERTY(EditAnywhere, Category = "Pattern")
	TObjectPtr<UCCLCombatDefinition> Attack;

	UPROPERTY(EditAnywhere, Category = "Pattern")
	FText Label;

	// The pattern can be chosen while health is at or below this fraction of maximum health.
	UPROPERTY(EditAnywhere, Category = "Pattern", meta = (ClampMin = "0", ClampMax = "1"))
	float MaxHealthFraction = 1.f;
};

// Content data for one enemy type. Encounter owners choose a definition instead of an archetype number.
UCLASS(BlueprintType)
class CCL_API UCCLEnemyDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

	// 내 클래스 함수
public:
	bool ValidateDefinition(TArray<FText>& Errors) const;

	// 에디터 전용
#if WITH_EDITOR
public:
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	// 프로퍼티
public:
	UPROPERTY(EditAnywhere, Category = "Identity")
	FText DisplayName;

	UPROPERTY(EditAnywhere, Category = "Resources", meta = (ClampMin = "1"))
	float InitialHealth = 80.f;

	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ClampMin = "0"))
	float WalkSpeed = 260.f;

	UPROPERTY(EditAnywhere, Category = "Awareness", meta = (ClampMin = "0"))
	float DetectionRadius = 1400.f;

	UPROPERTY(EditAnywhere, Category = "Awareness", meta = (ClampMin = "0"))
	float LeashRadius = 1800.f;

	// Eligible patterns are used in list order, continuing after the last chosen pattern.
	UPROPERTY(EditAnywhere, Category = "Attack")
	TArray<FCCLEnemyAttackPattern> Patterns;
};
