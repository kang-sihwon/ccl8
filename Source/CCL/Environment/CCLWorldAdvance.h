#pragma once

#include "CoreMinimal.h"

class FKSIWorldClock;
class FCCLLifeSimulation;
class FKSISurfaceSimulation;
class FKSIEcologySimulation;
class IKSIEcologySpace;
class FKSICelestialSystem;
class IKSISurfaceProvider;
struct FKSICelestialObserver;
struct FKSISurfaceOpening;
struct FKSIWorldStep;

namespace CCLWorldAdvance
{
	// The game-thread owner publishes clock, surface, ecology and life together; failure retains queued time.
	CCL_API bool Advance(FKSIWorldClock& Clock, FCCLLifeSimulation& Life,
		double MaxGameSeconds, double MaxWorldSeconds, FString& Error, int32 MaxLifeSlices = 256,
		FKSISurfaceSimulation* Surface = nullptr, const FKSICelestialSystem* Celestial = nullptr,
		const FKSICelestialObserver* Observer = nullptr, const IKSISurfaceProvider* Shelter = nullptr,
		const TArray<FKSISurfaceOpening>* Openings = nullptr, FKSIEcologySimulation* Ecology = nullptr,
		int32 MaxEcologyBoundaries = 256, const IKSIEcologySpace* Space = nullptr,
		const TFunction<bool(const FKSIWorldStep&, const FKSISurfaceSimulation*, FString&)>& Prepare = {});
}
