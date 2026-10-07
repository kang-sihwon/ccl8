#include "CCLAttachmentProfileCustomization.h"

#if WITH_EDITOR
#include "Items/CCLAttachmentProfile.h"
#include "Items/CCLItemDefinition.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "EditorViewportClient.h"
#include "PreviewScene.h"
#include "SEditorViewport.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "UObject/UObjectGlobals.h"

namespace
{
class SCCLAttachmentViewport : public SEditorViewport
{
  public:
	SLATE_BEGIN_ARGS(SCCLAttachmentViewport) {}
	SLATE_ARGUMENT(UCCLAttachmentProfile*, Profile)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args)
	{
		Profile = Args._Profile;
		Scene = MakeUnique<FPreviewScene>(FPreviewScene::ConstructionValues());
		Body = NewObject<USkeletalMeshComponent>();
		Scene->AddComponent(Body, FTransform::Identity);
		SEditorViewport::Construct(SEditorViewport::FArguments());
		Changed = FCoreUObjectDelegates::OnObjectPropertyChanged.AddLambda([this](UObject*, FPropertyChangedEvent&) { bNeedsRefresh = 1; });
		Refresh();
	}

	virtual ~SCCLAttachmentViewport() override
	{
		FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(Changed);
		// The client refers to the scene; release that reference before destroying it.
		if (Client.IsValid())
		{
			Client->Viewport = nullptr;
			Client.Reset();
		}
	}

	virtual void Tick(const FGeometry& Geometry, double Time, float Delta) override
	{
		SEditorViewport::Tick(Geometry, Time, Delta);
		if (bNeedsRefresh)
		{
			Refresh();
		}
	}

	FText GetStatus() const { return FText::FromString(Status); }

  protected:
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override
	{
		auto NewClient = MakeShared<FEditorViewportClient>(nullptr, Scene.Get(), SharedThis(this));
		NewClient->SetViewMode(VMI_Lit);
		NewClient->SetViewLocation(FVector(280.f, 240.f, 160.f));
		NewClient->SetViewRotation((FVector(0.f, 0.f, 90.f) - NewClient->GetViewLocation()).Rotation());
		NewClient->SetRealtime(true);
		return NewClient;
	}

  private:
	void Refresh()
	{
		bNeedsRefresh = 0;
		Status.Reset();
		if (Gear)
		{
			Scene->RemoveComponent(Gear);
			Gear->DestroyComponent();
			Gear = nullptr;
		}

		auto* Value = Profile.Get();
		Body->SetSkeletalMesh(Value ? Value->ReferenceMesh.Get() : nullptr);
		if (!Value || !Value->PreviewItem)
		{
			Status = TEXT("Select Reference Mesh, Preview Item and Preview Slot.");
			return;
		}

		TArray<FGameplayTag> Occupied;
		if (!CCLEquipment::GetOccupiedSlots(Value->PreviewItem, Value->PreviewSlot, Occupied) ||
			!Value->ValidateItem(Value->PreviewItem, Value->ReferenceMesh, Status))
		{
			if (Status.IsEmpty())
			{
				Status = TEXT("Item cannot occupy the selected slot.");
			}

			return;
		}

		const FGameplayTag Slot =
			CCLEquipment::IsTwoHanded(Value->PreviewItem) ? FGameplayTag(CCLItemTags::Slot_RightHand) : Value->PreviewSlot;
		const auto* Visual = Value->PreviewItem->FindFragment<FCCLItemFragment_Visual>();
		const auto* Binding = Visual ? Visual->FindAttachment(Slot) : nullptr;
		FName Socket;
		if (!Binding || !Value->Resolve(Value->ReferenceMesh, Binding->Point, Socket, Status))
		{
			if (Status.IsEmpty())
			{
				Status = TEXT("This item has no visual for this slot.");
			}

			return;
		}

		if (const auto* Skeletal = Value->PreviewItem->FindFragment<FCCLItemFragment_SkeletalVisual>(); Skeletal && Skeletal->EquippedMesh)
		{
			auto* Mesh = NewObject<USkeletalMeshComponent>();
			Mesh->SetSkeletalMesh(Skeletal->EquippedMesh);
			Gear = Mesh;
		}
		else if (Visual->DroppedMesh)
		{
			auto* Mesh = NewObject<UStaticMeshComponent>();
			Mesh->SetStaticMesh(Visual->DroppedMesh);
			Gear = Mesh;
		}

		if (Gear)
		{
			Scene->AddComponent(Gear, FTransform::Identity);
			Gear->AttachToComponent(Body, FAttachmentTransformRules::KeepRelativeTransform, Socket);
			Gear->SetRelativeTransform(Binding->Offset);
			Gear->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Status = FString::Printf(TEXT("%s -> %s (reference pose)"), *Binding->Point.ToString(), *Socket.ToString());
		}
	}

	TWeakObjectPtr<UCCLAttachmentProfile> Profile;
	TUniquePtr<FPreviewScene> Scene;
	USkeletalMeshComponent* Body = nullptr;
	UMeshComponent* Gear = nullptr;
	FDelegateHandle Changed;
	FString Status;
	uint8 bNeedsRefresh = 0;
};
} // namespace

TSharedRef<IDetailCustomization> FCCLAttachmentProfileCustomization::MakeInstance()
{
	return MakeShared<FCCLAttachmentProfileCustomization>();
}

void FCCLAttachmentProfileCustomization::CustomizeDetails(IDetailLayoutBuilder& Builder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	Builder.GetObjectsBeingCustomized(Objects);
	if (Objects.Num() != 1)
	{
		return;
	}

	auto* Profile = Cast<UCCLAttachmentProfile>(Objects[0].Get());
	if (!Profile)
	{
		return;
	}

	auto View = SNew(SCCLAttachmentViewport).Profile(Profile);
	auto& Category = Builder.EditCategory(TEXT("Preview"));
	Category.AddCustomRow(FText::FromString(TEXT("Attachment Preview")))
		.WholeRowContent()[SNew(SBox).HeightOverride(360.f).MinDesiredWidth(320.f)[View]];
	Category.AddCustomRow(FText::FromString(TEXT("Attachment Status")))
		.WholeRowContent()[SNew(STextBlock).AutoWrapText(true).Text(View, &SCCLAttachmentViewport::GetStatus)];
}
#endif
