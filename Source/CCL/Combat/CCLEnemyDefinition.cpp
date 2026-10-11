#include "CCLEnemyDefinition.h"

#include "Misc/DataValidation.h"

// 내 클래스 함수

bool UCCLEnemyDefinition::ValidateDefinition(TArray<FText>& Errors) const
{
	if (DisplayName.IsEmpty())
	{
		Errors.Add(FText::FromString(TEXT("Enemy definition needs a display name.")));
	}

	if (!FMath::IsFinite(InitialHealth) || InitialHealth < 1.f || !FMath::IsFinite(WalkSpeed) || WalkSpeed < 0.f)
	{
		Errors.Add(FText::FromString(TEXT("Enemy health must be at least 1 and speed must be finite and non-negative.")));
	}

	if (!FMath::IsFinite(DetectionRadius) || !FMath::IsFinite(LeashRadius) || DetectionRadius < 0.f || LeashRadius < DetectionRadius)
	{
		Errors.Add(FText::FromString(TEXT("Leash radius must be finite and not smaller than detection radius.")));
	}

	bool bFullHealthPattern = Patterns.IsEmpty();
	for (const auto& Pattern : Patterns)
	{
		if (!FMath::IsFinite(Pattern.MaxHealthFraction) || Pattern.MaxHealthFraction < 0.f || Pattern.MaxHealthFraction > 1.f)
		{
			Errors.Add(FText::FromString(TEXT("Pattern health fraction must be between 0 and 1.")));
		}

		bFullHealthPattern |= Pattern.MaxHealthFraction >= 1.f;
	}

	if (!bFullHealthPattern)
	{
		Errors.Add(FText::FromString(TEXT("At least one pattern must be available at full health.")));
	}

	return Errors.IsEmpty();
}

// 에디터 전용
#if WITH_EDITOR
EDataValidationResult UCCLEnemyDefinition::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult Parent = Super::IsDataValid(Context);
	TArray<FText> Errors;
	ValidateDefinition(Errors);
	for (const FText& Error : Errors)
	{
		Context.AddError(Error);
	}

	return Errors.IsEmpty() && Parent != EDataValidationResult::Invalid ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}
#endif
