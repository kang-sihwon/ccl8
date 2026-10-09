#include "CCLEnvironmentInputs.h"

#include "Misc/Crc.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace
{
	constexpr uint32 Magic = 0x49454343; // CCEI
	constexpr int32 MaximumBytes = 4 * 1024 * 1024;

	bool Name(FArchive& Ar, FName& Value)
	{
		// Bound before allocation; FName archive indices are process-local and must not be persisted.
		FString Text = Ar.IsSaving() ? Value.ToString() : FString();
		int32 Count = Text.Len();
		Ar << Count;
		if (Ar.IsError() || Count < 1 || Count > 128
			|| (Ar.IsLoading() && int64(Count) * int64(sizeof(uint16)) > Ar.TotalSize() - Ar.Tell()))
		{
			return false;
		}

		if (Ar.IsLoading())
		{
			Text.Reserve(Count);
		}

		for (int32 Index = 0; Index < Count; ++Index)
		{
			uint16 Character = Ar.IsSaving() ? static_cast<uint16>(Text[Index]) : 0;
			Ar << Character;
			if (Character == 0 || Ar.IsError())
			{
				return false;
			}

			if (Ar.IsLoading())
			{
				Text.AppendChar(static_cast<TCHAR>(Character));
			}
		}

		if (Ar.IsLoading())
		{
			Value = FName(Text);
		}

		return true;
	}

	bool Count(FArchive& Ar, int32& Value, int32 Maximum, int32 MinimumRecordBytes)
	{
		Ar << Value;
		return !Ar.IsError() && Value >= 0 && Value <= Maximum
			&& (!Ar.IsLoading() || int64(Value) * MinimumRecordBytes <= Ar.TotalSize() - Ar.Tell());
	}

	void Vector(FArchive& Ar, FVector3d& Value)
	{
		Ar << Value.X << Value.Y << Value.Z;
	}

	void Vector(FArchive& Ar, FVector2d& Value)
	{
		Ar << Value.X << Value.Y;
	}

	bool Serialize(FArchive& Ar, FCCLEnvironmentInputs& Inputs)
	{
		uint32 Tag = Magic;
		Ar << Tag << Inputs.Schema << Inputs.Revision;
		if (Ar.IsError() || Tag != Magic || Inputs.Schema != 1)
		{
			return false;
		}

		FCCLCelestialDefinitionData& Definition = Inputs.Celestial;
		Ar << Definition.DefinitionId << Definition.Version << Definition.Seed << Definition.EpochWorldSeconds;
		int32 BodyCount = Definition.Bodies.Num();
		if (!Count(Ar, BodyCount, 64, 109))
		{
			return false;
		}

		Definition.Bodies.SetNum(BodyCount);
		for (FCCLCelestialBodyDefinition& Body : Definition.Bodies)
		{
			if (!Name(Ar, Body.BodyId) || !Name(Ar, Body.ParentBodyId))
			{
				return false;
			}

			uint8 Kind = static_cast<uint8>(Body.Kind);
			Ar << Kind << Body.RadiusKm << Body.LuminosityWatts << Body.SemiMajorAxisKm << Body.Eccentricity;
			Ar << Body.OrbitalPeriodSeconds << Body.InclinationDegrees << Body.AscendingNodeDegrees << Body.PeriapsisDegrees;
			Ar << Body.MeanAnomalyDegrees << Body.ObliquityDegrees << Body.SpinPeriodSeconds << Body.SpinPhaseDegrees;
			Body.Kind = static_cast<ECCLCelestialKind>(Kind);
		}

		if (!Name(Ar, Inputs.Observer.BodyId))
		{
			return false;
		}

		Ar << Inputs.Observer.LatitudeDegrees << Inputs.Observer.LongitudeDegrees << Inputs.Observer.AltitudeMeters;
		Ar << Inputs.SurfaceRevision;
		int32 SurfaceCount = Inputs.Surfaces.Num();
		if (!Count(Ar, SurfaceCount, 4096, 165))
		{
			return false;
		}

		Inputs.Surfaces.SetNum(SurfaceCount);
		for (FCCLSurfacePatch& Patch : Inputs.Surfaces)
		{
			Ar << Patch.SurfaceId;
			if (!Name(Ar, Patch.BodyId) || !Name(Ar, Patch.MaterialId))
			{
				return false;
			}

			Vector(Ar, Patch.CenterMeters);
			Vector(Ar, Patch.Normal);
			Vector(Ar, Patch.TangentU);
			Vector(Ar, Patch.TangentV);
			Vector(Ar, Patch.HalfExtentsMeters);
			uint8 Kind = static_cast<uint8>(Patch.Kind);
			Ar << Kind << Patch.WaterDepthMeters << Patch.SnowDepthMeters;
			Ar << Patch.Transmission.Sun << Patch.Transmission.Precipitation << Patch.Transmission.Wind;
			Patch.Kind = static_cast<ECCLSurfaceKind>(Kind);
		}

		int32 OpeningCount = Inputs.Openings.Num();
		if (!Count(Ar, OpeningCount, 4096, 104))
		{
			return false;
		}

		Inputs.Openings.SetNum(OpeningCount);
		for (FCCLSurfaceOpening& Opening : Inputs.Openings)
		{
			Ar << Opening.OpeningId << Opening.SurfaceId;
			Vector(Ar, Opening.CenterUV);
			Vector(Ar, Opening.HalfExtentsMeters);
			Ar << Opening.OpenFraction << Opening.SpaceA << Opening.SpaceB;
		}

		return !Ar.IsError();
	}
}

bool FCCLEnvironmentInputsCodec::Encode(const FCCLEnvironmentInputs& Inputs, TArray<uint8>& OutBytes, FString& Error)
{
	if (!Validate(Inputs, Error))
	{
		return false;
	}

	FCCLEnvironmentInputs Copy = Inputs;
	TArray<uint8> Bytes;
	FMemoryWriter Writer(Bytes, true);
	if (!Serialize(Writer, Copy) || Bytes.Num() > MaximumBytes - 4)
	{
		Error = TEXT("Environment input exceeds its bounded serialization format.");
		return false;
	}

	uint32 CRC = FCrc::MemCrc32(Bytes.GetData(), Bytes.Num());
	Writer << CRC;
	OutBytes = MoveTemp(Bytes);
	Error.Reset();
	return true;
}

bool FCCLEnvironmentInputsCodec::Decode(const TArray<uint8>& Bytes, FCCLEnvironmentInputs& OutInputs, FString& Error)
{
	if (Bytes.Num() < 64 || Bytes.Num() > MaximumBytes)
	{
		Error = TEXT("Environment input byte count is invalid.");
		return false;
	}

	const int32 Size = Bytes.Num() - sizeof(uint32);
	uint32 CRC = 0;
	FMemory::Memcpy(&CRC, Bytes.GetData() + Size, sizeof(CRC));
	if (CRC != FCrc::MemCrc32(Bytes.GetData(), Size))
	{
		Error = TEXT("Environment input checksum mismatch.");
		return false;
	}

	TArray<uint8> Payload;
	Payload.Append(Bytes.GetData(), Size);
	FMemoryReader Reader(Payload, true);
	FCCLEnvironmentInputs Candidate;
	if (!Serialize(Reader, Candidate) || Reader.Tell() != Size)
	{
		Error = TEXT("Environment input is malformed or has an unsupported schema.");
		return false;
	}

	if (!Validate(Candidate, Error))
	{
		return false;
	}

	OutInputs = MoveTemp(Candidate);
	Error.Reset();
	return true;
}

bool FCCLEnvironmentInputsCodec::Prepare(const FCCLEnvironmentInputs& Inputs, double WorldSeconds,
	FCCLCelestialSystem& OutCelestial, FCCLSurfaceScene& OutSurfaces,
	FCCLCelestialObservation& OutObservation, FString& Error)
{
	if (Inputs.Schema != 1 || Inputs.Revision == 0 || Inputs.SurfaceRevision == 0)
	{
		Error = TEXT("Environment input schema or revision is invalid.");
		return false;
	}

	for (const FCCLCelestialBodyDefinition& Body : Inputs.Celestial.Bodies)
	{
		if (Body.BodyId.ToString().Len() > 128 || Body.ParentBodyId.ToString().Len() > 128)
		{
			Error = TEXT("Celestial identifiers exceed the persisted name limit.");
			return false;
		}
	}

	for (const FCCLSurfacePatch& Surface : Inputs.Surfaces)
	{
		if (Surface.MaterialId.ToString().Len() > 128)
		{
			Error = TEXT("Surface material identifier exceeds the persisted name limit.");
			return false;
		}
	}

	FCCLCelestialSystem Celestial;
	FCCLSurfaceScene Surfaces;
	FCCLCelestialObservation Observation;
	if (!Celestial.Initialize(Inputs.Celestial, Error)
		|| !Celestial.Observe(WorldSeconds, Inputs.Observer, Observation, Error)
		|| !Surfaces.Replace(Inputs.Surfaces, Inputs.Openings, Inputs.SurfaceRevision, Error))
	{
		return false;
	}

	for (const FCCLSurfacePatch& Surface : Inputs.Surfaces)
	{
		if (!Inputs.Celestial.Bodies.ContainsByPredicate([&Surface](const auto& Body)
			{ return Body.BodyId == Surface.BodyId && Body.Kind != ECCLCelestialKind::Star; }))
		{
			Error = TEXT("Surface refers to a missing or luminous body.");
			return false;
		}
	}

	OutCelestial = MoveTemp(Celestial);
	OutSurfaces = MoveTemp(Surfaces);
	OutObservation = MoveTemp(Observation);
	Error.Reset();
	return true;
}

bool FCCLEnvironmentInputsCodec::Validate(const FCCLEnvironmentInputs& Inputs, FString& Error)
{
	FCCLCelestialSystem Celestial;
	FCCLSurfaceScene Surfaces;
	FCCLCelestialObservation Observation;
	return Prepare(Inputs, FMath::Max(0., Inputs.Celestial.EpochWorldSeconds), Celestial, Surfaces, Observation, Error);
}

bool FCCLEnvironmentInputsCodec::CheckDefinition(const FCCLEnvironmentInputs& Inputs, const FGuid& ExpectedDefinitionId,
	int32 ExpectedVersion, FString& Error)
{
	if (!ExpectedDefinitionId.IsValid() || ExpectedVersion < 1 || Inputs.Celestial.DefinitionId != ExpectedDefinitionId
		|| Inputs.Celestial.Version != ExpectedVersion)
	{
		Error = TEXT("Saved environment uses a different celestial definition or version.");
		return false;
	}

	Error.Reset();
	return true;
}

FCCLEnvironmentInputs FCCLEnvironmentInputsCodec::MakeDefault(int32 Seed)
{
	FCCLEnvironmentInputs Result;
	Result.Celestial = FCCLCelestialSystem::MakeDefaultDefinition(Seed);
	Result.Observer.BodyId = TEXT("World");
	Result.Observer.LatitudeDegrees = 45.;
	return Result;
}
