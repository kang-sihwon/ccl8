#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "CCLUITypes.generated.h"

UENUM(BlueprintType)
enum class ECCLUIInstancePolicy : uint8
{
	Single,
	PerContext,
	Multiple
};

UENUM(BlueprintType)
enum class ECCLUILayerLayout : uint8
{
	Overlay,
	Stack,
	Queue
};

UENUM(BlueprintType)
enum class ECCLUIScope : uint8
{
	World,
	LocalPlayer
};

UENUM(BlueprintType)
enum class ECCLUIInputPolicy : uint8
{
	Inherit,
	Game,
	Menu,
	GameAndUI
};

USTRUCT(BlueprintType)
struct CCL_API FCCLUIViewHandle
{
	GENERATED_BODY()

	bool IsValid() const { return Id.IsValid(); }
	bool operator==(const FCCLUIViewHandle& Other) const { return Id == Other.Id; }
	friend uint32 GetTypeHash(const FCCLUIViewHandle& Value) { return GetTypeHash(Value.Id); }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGuid Id;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLUIRegistrationHandle
{
	GENERATED_BODY()

	bool IsValid() const { return Id.IsValid(); }
	bool operator==(const FCCLUIRegistrationHandle& Other) const { return Id == Other.Id; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGuid Id;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLUIRequestHandle
{
	GENERATED_BODY()

	bool IsValid() const { return Id.IsValid(); }
	bool operator==(const FCCLUIRequestHandle& Other) const { return Id == Other.Id; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGuid Id;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLUILayerDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "UI.Layer"))
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ECCLUILayerLayout Layout = ECCLUILayerLayout::Overlay;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 ZOrder = 0;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLUIExtensionDefinition
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
