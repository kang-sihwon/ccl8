#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CCLItemTags.h"
#include "CCLAttachmentProfile.generated.h"

class USkeletalMesh;
class UCCLItemDefinition;

USTRUCT(BlueprintType)
struct CCL_API FCCLAttachmentSocket
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (Categories = "Attachment"))
	FGameplayTag Point;

	UPROPERTY(EditAnywhere, meta = (GetOptions = "GetSocketNames"))
	FName Socket;
};

UCLASS(BlueprintType)
class CCL_API UCCLAttachmentProfile : public UDataAsset
{
	GENERATED_BODY()

  public:
	bool Resolve(const USkeletalMesh* Mesh, FGameplayTag Point, FName& OutSocket, FString& Error) const;
	bool ValidateItem(const UCCLItemDefinition* Item, const USkeletalMesh* Mesh, FString& Error) const;
	bool ValidateProfile(TArray<FText>& Errors) const;

#if WITH_EDITOR
  public:
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;

	UFUNCTION()
	TArray<FString> GetSocketNames() const;
#endif

#if WITH_EDITORONLY_DATA
  public:
	UPROPERTY(EditAnywhere, Category = "Preview")
	TObjectPtr<UCCLItemDefinition> PreviewItem;

	UPROPERTY(EditAnywhere, Category = "Preview", meta = (Categories = "Equipment.Slot"))
	FGameplayTag PreviewSlot = CCLItemTags::Slot_RightHand;
#endif

  public:
	UPROPERTY(EditAnywhere, Category = "Attachment")
	TObjectPtr<USkeletalMesh> ReferenceMesh;

	UPROPERTY(EditAnywhere, Category = "Attachment", meta = (TitleProperty = "Point"))
	TArray<FCCLAttachmentSocket> Bindings;
};
