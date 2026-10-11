#include "CCLFoodEnvironment.h"

#include "CCLInventorySnapshot.h"
#include "Environment/CCLSurfaceSimulation.h"

bool CCLFoodEnvironment::Validate(const FCCLFoodEnvironmentBinding& B, double Time, FString& Error)
{
	if (uint8(B.Source) > uint8(ECCLFoodEnvironment::AirSpace) || !FMath::IsFinite(Time) || Time < 0. ||
		!FMath::IsFinite(B.SampleWorldSeconds) || B.SampleWorldSeconds < 0. || B.SampleWorldSeconds > Time ||
		!CCLFood::ValidateStorage(B.Conditions) || B.SourceId.IsValid() != B.IsDynamic() ||
		(!B.IsDynamic() && B.SampleWorldSeconds != 0.))
	{
		Error = TEXT("Invalid food environment binding or future sample.");
		return false;
	}
	return true;
}

bool CCLFoodEnvironment::Sample(FCCLFoodEnvironmentBinding& B, const FCCLSurfaceSimulation& S, FString& Error)
{
	if (!S.IsInitialized() || !Validate(B, S.GetWorldSeconds(), Error))
	{
		return false;
	}
	if (!B.IsDynamic())
	{
		return true;
	}
	double Temperature = 0., VaporDensity = 0.;
	if (B.Source == ECCLFoodEnvironment::WeatherRegion)
	{
		const auto* Region = S.FindRegion(B.SourceId);
		if (!Region)
		{
			Error = TEXT("Food climate region is missing.");
			return false;
		}
		Temperature = Region->Forcing.TemperatureCelsius;
		VaporDensity = Region->Weather.VaporMeters;
	}
	else
	{
		const auto* Space = S.GetAirSimulation().FindSpace(B.SourceId);
		if (!Space || !Space->Validate(Error))
		{
			Error = TEXT("Food indoor air space is missing or invalid.");
			return false;
		}
		Temperature = Space->TemperatureCelsius();
		VaporDensity = Space->Contents.VaporKg / Space->VolumeCubicMeters;
	}
	if (!FMath::IsFinite(Temperature) || !FMath::IsFinite(VaporDensity) || VaporDensity < 0.)
	{
		Error = TEXT("Food environment sample is invalid.");
		return false;
	}
	auto Candidate = B;
	Candidate.Conditions.TemperatureCelsius = FMath::Clamp(Temperature, -80., 80.);
	// Same vapor-capacity approximation as FCCLWeatherModel, including its 1000 m mixing-height convention.
	const double Capacity = .005 * FMath::Exp(FMath::Clamp(Temperature * .06, -10., 10.));
	Candidate.Conditions.RelativeHumidity = FMath::Clamp(VaporDensity / Capacity, 0., 1.);
	Candidate.SampleWorldSeconds = S.GetWorldSeconds();
	if (!Validate(Candidate, S.GetWorldSeconds(), Error))
	{
		return false;
	}
	B = Candidate;
	return true;
}

bool CCLFoodEnvironment::Advance(FCCLInventorySnapshot& Record, const FCCLSurfaceSimulation& S, double From,
								 FString& Error)
{
	const double To = S.GetWorldSeconds();
	if (!Validate(Record.Environment, From, Error) || !FMath::IsFinite(From) || To < From ||
		To > FMath::FloorToDouble(From) + 1. + 1.e-9)
	{
		Error = TEXT("Food environment advance requires a single committed sampling interval.");
		return false;
	}
	if (!Record.Environment.IsDynamic() || To == From)
	{
		return true;
	}
	auto Candidate = Record;
	// Check the source on every prepare, but publish a new held sample only at an integer world boundary.
	auto Sampled = Candidate.Environment;
	if (!Sample(Sampled, S, Error))
	{
		return false;
	}
	const bool bBoundary = FMath::IsNearlyEqual(To, FMath::RoundToDouble(To), 1.e-9);
	for (auto& Item : Candidate.Items)
	{
		FCCLFoodState Food;
		if (!CCLFood::Decode(Item.Food, Food, Error))
		{
			return false;
		}
		if (!Food.IsPresent())
		{
			continue;
		}
		if (Food.WorldId != S.GetWorldId() || Food.UpdatedWorldSeconds > From ||
			!CCLFood::ChangeStorage(Food, Candidate.Environment.Conditions, From, Error) ||
			!CCLFood::Advance(Food, To, Error) ||
			(bBoundary && !CCLFood::ChangeStorage(Food, Sampled.Conditions, To, Error)) ||
			!CCLFood::Encode(Food, Item.Food, Error))
		{
			return false;
		}
	}
	if (bBoundary)
	{
		Candidate.Environment = Sampled;
	}
	Record = MoveTemp(Candidate);
	return true;
}

bool CCLFoodEnvironment::ResolveAt(const FVector& Position, const FCCLSurfaceSimulation& Surface,
								   FCCLFoodEnvironmentBinding& Binding, FString& Error)
{
	if (Position.ContainsNaN() || !Surface.IsInitialized())
	{
		Error = TEXT("Food position or environment is unavailable.");
		return false;
	}
	FCCLFoodEnvironmentBinding Candidate;
	double Best = TNumericLimits<double>::Max();
	auto Better = [&Candidate, &Best](double Size, FGuid Id)
	{ return Size < Best || (Size == Best && Id < Candidate.SourceId); };
	for (const auto& Pair : Surface.GetAirSimulation().GetSpaces())
	{
		const auto& Space = Pair.Value;
		const FVector Extent = Space.MaximumMeters - Space.MinimumMeters;
		const double Size = Extent.X * Extent.Y * Extent.Z;
		if (Space.Contains(Position) && Better(Size, Pair.Key))
		{
			Candidate.Source = ECCLFoodEnvironment::AirSpace;
			Candidate.SourceId = Pair.Key;
			Best = Size;
		}
	}
	if (!Candidate.IsDynamic())
	{
		for (const auto& Pair : Surface.GetRegions())
		{
			const auto& Grid = Pair.Value;
			const FVector Local = Position - Grid.OriginMeters;
			const double Width = Grid.Size.X * Grid.SpacingMeters;
			const double Height = Grid.Size.Y * Grid.SpacingMeters;
			if (Local.X >= 0. && Local.Y >= 0. && Local.X < Width && Local.Y < Height &&
				Better(Width * Height, Pair.Key))
			{
				Candidate.Source = ECCLFoodEnvironment::WeatherRegion;
				Candidate.SourceId = Pair.Key;
				Best = Width * Height;
			}
		}
	}
	if (!Sample(Candidate, Surface, Error))
	{
		return false;
	}
	Binding = Candidate;
	return true;
}

bool CCLFoodEnvironment::RelocatePickup(FCCLInventorySnapshot& Record, const FCCLSurfaceSimulation& Surface,
										FString& Error)
{
	if (Record.Owner != ECCLInventoryOwner::FoodPickup || Record.Items.IsEmpty())
	{
		return true;
	}
	FCCLFoodEnvironmentBinding Binding;
	if (!ResolveAt(Record.Transform.GetLocation() / 100., Surface, Binding, Error))
	{
		return false;
	}
	if (Binding.IsDynamic() && Binding.Source == Record.Environment.Source && Binding.SourceId == Record.Environment.SourceId)
	{
		return true;
	}
	auto Candidate = Record;
	for (auto& Item : Candidate.Items)
	{
		FCCLFoodState Food;
		if (!CCLFood::Decode(Item.Food, Food, Error) || Food.WorldId != Surface.GetWorldId())
		{
			return false;
		}
		if (Food.Storage == Binding.Conditions)
		{
			continue;
		}
		if (!CCLFood::ChangeStorage(Food, Binding.Conditions, Surface.GetWorldSeconds(), Error) ||
			!CCLFood::Encode(Food, Item.Food, Error))
		{
			return false;
		}
	}
	Candidate.Environment = Binding;
	Record = MoveTemp(Candidate);
	return true;
}
