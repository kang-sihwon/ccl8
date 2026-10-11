#pragma once

#include "CoreMinimal.h"

class FCCLWorldClock;
class FCCLLifeSimulation;
class FCCLSurfaceSimulation;
class FCCLEcologySimulation;
class ICCLEcologySpace;
class FCCLCelestialSystem;
class ICCLSurfaceProvider;
struct FCCLCelestialObserver;
struct FCCLSurfaceOpening;
struct FCCLWorldStep;

namespace CCLWorldAdvance
{
	// The game-thread owner publishes clock, surface, ecology and life together; failure retains queued time.
	CCL_API bool Advance(FCCLWorldClock& Clock, FCCLLifeSimulation& Life,
		double MaxGameSeconds, double MaxWorldSeconds, FString& Error, int32 MaxLifeSlices = 256,
		FCCLSurfaceSimulation* Surface = nullptr, const FCCLCelestialSystem* Celestial = nullptr,
		const FCCLCelestialObserver* Observer = nullptr, const ICCLSurfaceProvider* Shelter = nullptr,
		const TArray<FCCLSurfaceOpening>* Openings = nullptr, FCCLEcologySimulation* Ecology = nullptr,
		int32 MaxEcologyBoundaries = 256, const ICCLEcologySpace* Space = nullptr,
		const TFunction<bool(const FCCLWorldStep&, const FCCLSurfaceSimulation*, FString&)>& Prepare = {});
}
