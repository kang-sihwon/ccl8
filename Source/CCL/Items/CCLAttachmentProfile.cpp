#include "CCLAttachmentProfile.h"

#include "CCLItemDefinition.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Misc/DataValidation.h"

bool UCCLAttachmentProfile::Resolve(const USkeletalMesh* Mesh, FGameplayTag Point, FName& OutSocket, FString& Error) const
{
	OutSocket = NAME_None;
	Error.Reset();
	if (!Mesh || !ReferenceMesh || Mesh->GetSkeleton() != ReferenceMesh->GetSkeleton())
	{
		Error = TEXT("Attachment profile requires a mesh with the reference skeleton.");
		return false;
	}

	const FCCLAttachmentSocket* Binding = nullptr;
	for (const auto& Value : Bindings)
	{
		if (Value.Point == Point)
		{
			if (Binding)
			{
				Error = FString::Printf(TEXT("Duplicate attachment point: %s"), *Point.ToString());
				return false;
			}

			Binding = &Value;
		}
	}

	const auto* Socket = Binding ? Mesh->FindSocket(Binding->Socket) : nullptr;
	if (!Point.IsValid() || !Socket || Mesh->GetRefSkeleton().FindBoneIndex(Socket->BoneName) == INDEX_NONE)
	{
		Error = FString::Printf(TEXT("Missing attachment mapping, explicit socket or socket bone: %s"), *Point.ToString());
		return false;
	}

	OutSocket = Binding->Socket;
	return true;
}

bool UCCLAttachmentProfile::ValidateItem(const UCCLItemDefinition* Item, const USkeletalMesh* Mesh, FString& Error) const
{
	Error.Reset();
	TArray<FText> Errors;
	if (!Item || !Item->ValidateDefinition(Errors))
	{
		Error = Errors.IsEmpty() ? TEXT("Missing item.") : Errors[0].ToString();
		return false;
	}

	const auto* Visual = Item->FindFragment<FCCLItemFragment_Visual>();
	if (!Visual)
	{
		return true;
	}

	for (const auto& Binding : Visual->Attachments)
	{
		FName Socket;
		if (!Resolve(Mesh, Binding.Point, Socket, Error))
		{
			return false;
		}
	}

	return true;
}

bool UCCLAttachmentProfile::ValidateProfile(TArray<FText>& Errors) const
{
	if (!ReferenceMesh || Bindings.IsEmpty())
	{
		Errors.Add(FText::FromString(TEXT("ReferenceMesh and attachment bindings are required.")));
	}

	TSet<FGameplayTag> Seen;
	const FGameplayTag Root = FGameplayTag::RequestGameplayTag(TEXT("Attachment"));
	for (const auto& Binding : Bindings)
	{
		FName Socket;
		FString Error;
		if (Seen.Contains(Binding.Point) || !Binding.Point.MatchesTag(Root) || Binding.Point == Root)
		{
			Errors.Add(FText::FromString(TEXT("Attachment point must be unique and below Attachment.")));
		}

		if (!Resolve(ReferenceMesh, Binding.Point, Socket, Error))
		{
			Errors.Add(FText::FromString(Error));
		}

		Seen.Add(Binding.Point);
	}

	return Errors.IsEmpty();
}

#if WITH_EDITOR
EDataValidationResult UCCLAttachmentProfile::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult Parent = Super::IsDataValid(Context);
	TArray<FText> Errors;
	ValidateProfile(Errors);
	if (PreviewItem)
	{
		FString Error;
		TArray<FGameplayTag> Slots;
		if (!ValidateItem(PreviewItem, ReferenceMesh, Error))
		{
			Errors.Add(FText::FromString(Error));
		}

		if (!CCLEquipment::GetOccupiedSlots(PreviewItem, PreviewSlot, Slots))
		{
			Errors.Add(FText::FromString(TEXT("Preview slot is not allowed by the item.")));
		}
	}

	for (const auto& Error : Errors)
	{
		Context.AddError(Error);
	}

	return Errors.IsEmpty() && Parent != EDataValidationResult::Invalid ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}

TArray<FString> UCCLAttachmentProfile::GetSocketNames() const
{
	TArray<FString> Names;
	if (ReferenceMesh)
	{
		for (const auto* Socket : ReferenceMesh->GetActiveSocketList())
		{
			if (Socket)
			{
				Names.AddUnique(Socket->SocketName.ToString());
			}
		}
	}

	Names.Sort();
	return Names;
}
#endif
