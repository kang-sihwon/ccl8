#include "CCLAgentSnapshot.h"

#include "Kismet/GameplayStatics.h"
#include "Misc/Crc.h"

bool UCCLAgentSnapshot::Encode(const FCCLAgentStore& Store, TArray<uint8>& Bytes)
{
	auto* Snapshot = NewObject<UCCLAgentSnapshot>();
	if (!Store.Snapshot(Snapshot->Agents))
	{
		return false;
	}

	TArray<uint8> Payload;
	if (!UGameplayStatics::SaveGameToMemory(Snapshot, Payload) || Payload.Num() > 16 * 1024 * 1024)
	{
		return false;
	}

	const uint32 Checksum = FCrc::MemCrc32(Payload.GetData(), Payload.Num());
	Payload.Append(reinterpret_cast<const uint8*>(&Checksum), sizeof(Checksum));
	Bytes = MoveTemp(Payload);
	return true;
}

bool UCCLAgentSnapshot::Decode(const TArray<uint8>& Bytes, FCCLAgentStore& Store,
	const FCCLFeatureRegistry& Registry, FString& Error)
{
	if (Bytes.Num() < 32 || Bytes.Num() > 16 * 1024 * 1024 + 4)
	{
		Error = TEXT("Invalid agent snapshot size.");
		return false;
	}

	const int32 PayloadSize = Bytes.Num() - sizeof(uint32);
	uint32 Checksum;
	FMemory::Memcpy(&Checksum, Bytes.GetData() + PayloadSize, sizeof(Checksum));
	if (Checksum != FCrc::MemCrc32(Bytes.GetData(), PayloadSize))
	{
		Error = TEXT("Agent snapshot checksum mismatch.");
		return false;
	}

	TArray<uint8> Payload;
	Payload.Append(Bytes.GetData(), PayloadSize);
	const auto* Snapshot = Cast<UCCLAgentSnapshot>(UGameplayStatics::LoadGameFromMemory(Payload));
	if (!Snapshot || Snapshot->Version != 1 || Snapshot->Agents.Num() > 10000)
	{
		Error = TEXT("Unsupported agent snapshot.");
		return false;
	}

	return Store.Replace(Snapshot->Agents, Registry, Error);
}
