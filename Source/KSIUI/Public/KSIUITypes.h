#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KSIUITypes.generated.h"

UENUM(BlueprintType)
enum class EKSIUIInstancePolicy : uint8
{
	Single,
	PerContext,
	Multiple
};

UENUM(BlueprintType)
enum class EKSIUILayerLayout : uint8
{
	Overlay,
	Stack,
	Queue
};

UENUM(BlueprintType)
enum class EKSIUIScope : uint8
{
	World,
	LocalPlayer
};

UENUM(BlueprintType)
enum class EKSIUIInputPolicy : uint8
{
	Inherit,
	Game,
	Menu,
	GameAndUI
};

USTRUCT(BlueprintType)
struct KSIUI_API FKSIUIViewHandle
{
	GENERATED_BODY()

	bool IsValid() const { return Id.IsValid(); }
	bool operator==(const FKSIUIViewHandle& Other) const { return Id == Other.Id; }
	friend uint32 GetTypeHash(const FKSIUIViewHandle& Value) { return GetTypeHash(Value.Id); }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGuid Id;
};

USTRUCT(BlueprintType)
struct KSIUI_API FKSIUIRegistrationHandle
{
	GENERATED_BODY()

	bool IsValid() const { return Id.IsValid(); }
	bool operator==(const FKSIUIRegistrationHandle& Other) const { return Id == Other.Id; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGuid Id;
};

USTRUCT(BlueprintType)
struct KSIUI_API FKSIUIRequestHandle
{
	GENERATED_BODY()

	bool IsValid() const { return Id.IsValid(); }
	bool operator==(const FKSIUIRequestHandle& Other) const { return Id == Other.Id; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGuid Id;
};

USTRUCT(BlueprintType)
struct KSIUI_API FKSIUILayerDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "UI.Layer"))
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EKSIUILayerLayout Layout = EKSIUILayerLayout::Overlay;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 ZOrder = 0;
};

USTRUCT(BlueprintType)
struct KSIUI_API FKSIUIExtensionDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "UI.Extension"))
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "UI.Layer"))
	FGameplayTag Layer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D Alignment = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FMargin Padding;
};
