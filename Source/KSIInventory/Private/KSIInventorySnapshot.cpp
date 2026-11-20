#include "KSIInventoryComponent.h"

#include "KSIInventoryEdit.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace
{
constexpr int32 MaxSnapshotBytes = 8 * 1024 * 1024;
constexpr int32 MaxStateBytes = 64 * 1024;
constexpr uint32 InventoryMagic = 0x4B534949;

bool DefinitionName(FArchive& Ar, FString& Name)
{
	if (Ar.IsSaving())
	{
		FTCHARToUTF8 Utf8(*Name);
		int32 Size = Utf8.Length();
		if (Size < 1 || Size > 1024)
		{
			return false;
		}
		Ar << Size;
		Ar.Serialize(const_cast<ANSICHAR*>(Utf8.Get()), Size);
	}
	else
	{
		int32 Size = 0;
		Ar << Size;
		if (Ar.IsError() || Size < 1 || Size > 1024 || Size > Ar.TotalSize() - Ar.Tell())
		{
			return false;
		}
		TArray<ANSICHAR> Utf8;
		Utf8.SetNumZeroed(Size + 1);
		Ar.Serialize(Utf8.GetData(), Size);
		Name = UTF8_TO_TCHAR(Utf8.GetData());
		if (FTCHARToUTF8(*Name).Length() != Size)
		{
			return false;
		}
	}
	return !Ar.IsError();
}
}

bool UKSIInventoryComponent::CanUsePlainTransfer(const FKSIInventoryEntry& Entry) const
{
	return !Entry.State.IsValid();
}

bool UKSIInventoryComponent::TransferTo(FGuid Id, int32 Quantity, UKSIInventoryComponent& Destination)
{
	if (&Destination == this || GetWorld() != Destination.GetWorld() || Quantity < 1)
	{
		return false;
	}
	FKSIInventoryEdit From(*this);
	FKSIInventoryEdit To(Destination);
	FKSIInventoryEntry* Source = From.Find(Id);
	if (!From.IsOpen() || !To.IsOpen() || !Source || Source->Slot < 0 || Source->Quantity < Quantity || To.Find(Id) || !CanUsePlainTransfer(*Source) || !Destination.CanUsePlainTransfer(*Source))
	{
		return false;
	}
	const int32 Slot = To.FindFreeSlot();
	if (Slot == INDEX_NONE)
	{
		return false;
	}
	FKSIInventoryEntry& Added = To.GetEntries().Add_GetRef(*Source);
	Added.Quantity = Quantity;
	Added.Slot = Slot;
	if (Quantity != Source->Quantity)
	{
		Added.Id = FGuid::NewGuid();
	}
	Source->Quantity -= Quantity;
	From.GetEntries().RemoveAll([](const FKSIInventoryEntry& Entry)
								{ return Entry.Quantity == 0; });
	if (!From.Validate() || !To.Validate())
	{
		return false;
	}
	// 두 목록을 모두 확정한 뒤에 알린다.
	verify(From.Commit());
	verify(To.Commit());
	From.Publish();
	To.Publish();
	return true;
}

bool UKSIInventoryComponent::EncodeState(const FKSIInventoryEntry& Entry, TArray<uint8>& OutBytes) const
{
	OutBytes.Reset();
	return !Entry.State.IsValid();
}

bool UKSIInventoryComponent::DecodeState(const TArray<uint8>& Bytes, FKSIInventoryEntry& Entry) const
{
	Entry.State.Reset();
	return Bytes.IsEmpty();
}

bool UKSIInventoryComponent::Capture(TArray<uint8>& OutBytes) const
{
	if (IsEditOpen() || !CanRestore(List.Entries))
	{
		return false;
	}
	TArray<uint8> Bytes;
	FMemoryWriter Ar(Bytes);
	uint32 Magic = InventoryMagic;
	uint32 Version = 1;
	int32 Count = List.Entries.Num();
	int32 SavedCapacity = Capacity;
	Ar << Magic << Version << SavedCapacity << Count;
	TMap<FString, const UKSIItemDefinition*> Definitions;
	for (const auto& Entry : List.Entries)
	{
		FGuid Id = Entry.Id;
		int32 Quantity = Entry.Quantity;
		int32 Slot = Entry.Slot;
		const FPrimaryAssetId DefinitionId = Entry.Definition->GetPrimaryAssetId();
		if (!DefinitionId.IsValid())
		{
			return false;
		}
		FString Name = DefinitionId.ToString();
		const auto* Existing = Definitions.Find(Name);
		if (Existing && *Existing != Entry.Definition)
		{
			return false;
		}
		Definitions.Add(Name, Entry.Definition);
		TArray<uint8> State;
		if (!EncodeState(Entry, State) || State.Num() > MaxStateBytes || !DefinitionName(Ar, Name))
		{
			return false;
		}
		int32 Size = State.Num();
		Ar << Id << Quantity << Slot << Size;
		Ar.Serialize(State.GetData(), Size);
		if (Ar.IsError() || Bytes.Num() > MaxSnapshotBytes)
		{
			return false;
		}
	}
	OutBytes = MoveTemp(Bytes);
	return true;
}

bool UKSIInventoryComponent::RestoreSnapshot(const TArray<uint8>& Bytes, TFunctionRef<UKSIItemDefinition*(const FString&)> ResolveDefinition)
{
	if (Bytes.Num() < 16 || Bytes.Num() > MaxSnapshotBytes)
	{
		return false;
	}
	FKSIInventoryEdit Edit(*this);
	if (!Edit.IsOpen())
	{
		return false;
	}
	FMemoryReader Ar(Bytes);
	uint32 Magic = 0, Version = 0;
	int32 SavedCapacity = 0, Count = 0;
	Ar << Magic << Version << SavedCapacity << Count;
	if (Magic != InventoryMagic || Version != 1 || SavedCapacity != Capacity || Count < 0 || Count > Capacity + 128)
	{
		return false;
	}
	TArray<FKSIInventoryEntry> Candidate;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		FString Name;
		if (!DefinitionName(Ar, Name))
		{
			return false;
		}
		FKSIInventoryEntry Entry;
		int32 Size = 0;
		Ar << Entry.Id << Entry.Quantity << Entry.Slot << Size;
		if (Ar.IsError() || Size < 0 || Size > MaxStateBytes || Size > Ar.TotalSize() - Ar.Tell())
		{
			return false;
		}
		TArray<uint8> State;
		State.SetNumUninitialized(Size);
		Ar.Serialize(State.GetData(), Size);
		Entry.Definition = ResolveDefinition(Name);
		if (!Entry.Definition || Entry.Definition->GetPrimaryAssetId().ToString() != Name || !DecodeState(State, Entry))
		{
			return false;
		}
		Candidate.Add(MoveTemp(Entry));
	}
	if (Ar.IsError() || Ar.Tell() != Ar.TotalSize())
	{
		return false;
	}
	Edit.Replace(MoveTemp(Candidate));
	return Edit.Commit();
}
