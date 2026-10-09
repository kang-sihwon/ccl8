#pragma once

#include "CoreMinimal.h"

class FCCLWorldClock;
class FCCLLifeSimulation;

namespace CCLWorldAdvance
{
	// The game-thread owner publishes clock and life together; failure retains queued time.
	CCL_API bool Advance(FCCLWorldClock& Clock, FCCLLifeSimulation& Life,
		double MaxGameSeconds, double MaxWorldSeconds, FString& Error, int32 MaxLifeSlices = 256);
}
