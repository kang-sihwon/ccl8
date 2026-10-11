#pragma once

#include "CoreMinimal.h"
#include "Misc/Crc.h"

#if WITH_DEV_AUTOMATION_TESTS
// Test fixtures strip the actual v6 fields before exercising older decoders.
inline void CCLMakeEcologyV5Fixture(TArray<uint8>& Bytes)
{
	TArray<uint8> Legacy;
	int32 Offset = 0;
	auto Copy = [&](int32 Size)
	{
		check(Size >= 0 && Offset + Size <= Bytes.Num());
		Legacy.Append(Bytes.GetData() + Offset, Size);
		Offset += Size;
	};
	auto Count = [&]()
	{
		int32 Value;
		check(Offset + 4 <= Bytes.Num());
		FMemory::Memcpy(&Value, Bytes.GetData() + Offset, 4);
		Copy(4);
		check(Value >= 0);
		return Value;
	};
	Copy(32);
	int32 N = Count();
	for (int32 I = 0; I < N; ++I)
	{
		Copy(41);
		Offset += 28;
	}
	N = Count();
	Copy(N * 73);
	N = Count();
	for (int32 I = 0; I < N; ++I)
	{
		Copy(96);
		const int32 Cohorts = Count();
		for (int32 C = 0; C < Cohorts; ++C)
		{
			Copy(32);
			Offset += 24;
		}
		Offset += 8;
	}
	N = Count();
	for (int32 I = 0; I < N; ++I)
	{
		Copy(82);
		Offset += 24;
	}
	Copy(Bytes.Num() - Offset);
	uint32 Version = 5;
	FMemory::Memcpy(Legacy.GetData() + 4, &Version, 4);
	const uint32 Crc = FCrc::MemCrc32(Legacy.GetData(), Legacy.Num() - 4);
	FMemory::Memcpy(Legacy.GetData() + Legacy.Num() - 4, &Crc, 4);
	Bytes = MoveTemp(Legacy);
}
#endif
