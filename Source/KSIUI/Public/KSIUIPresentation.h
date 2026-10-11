#pragma once

#include "CoreMinimal.h"
#include "KSIUITypes.h"
#include "KSIUIPresentation.generated.h"

USTRUCT(BlueprintType)
struct KSIUI_API FKSIUIPresentationHandle
{
	GENERATED_BODY()

	bool IsValid() const { return Id.IsValid(); }
	bool operator==(const FKSIUIPresentationHandle& Other) const { return Id == Other.Id; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGuid Id;
};

USTRUCT(BlueprintType)
struct KSIUI_API FKSIUIViewPresentation
{
	GENERATED_BODY()

	bool IsValid() const { return FMath::IsFinite(Opacity) && Opacity >= 0.f && Opacity <= 1.f; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 bVisible = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 bInputEnabled = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 bUpdatesEnabled = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "1"))
	float Opacity = 1.f;
};

USTRUCT(BlueprintType)
struct KSIUI_API FKSIUIPresentationDefinition
{
	GENERATED_BODY()

	bool IsValid() const;
	bool Matches(const FGameplayTagContainer& ViewGroups) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Categories = "UI.Group"))
	FGameplayTagContainer Groups;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 bAllViews = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 bHide = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 bDisableInput = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 bSuspendUpdates = 0;

	// This option applies to gameplay input for the entire owning local player.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 bBlockGameplay = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "1"))
	float Opacity = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	float FadeSeconds = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EKSIUIScope Scope = EKSIUIScope::World;
};

USTRUCT()
struct KSIUI_API FKSIUIPresentationRequest
{
	GENERATED_BODY()

	UPROPERTY()
	FKSIUIPresentationDefinition Definition;

	UPROPERTY()
	TWeakObjectPtr<UObject> Owner;

	UPROPERTY()
	TWeakObjectPtr<UWorld> World;
};
