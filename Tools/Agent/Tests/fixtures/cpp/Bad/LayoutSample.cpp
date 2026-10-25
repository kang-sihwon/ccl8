#include "Engine/World.h"
#include "LayoutSample.h"

ALayoutSample::ALayoutSample() { }

void ALayoutSample::BeginPlay()
{

	if (!GetWorld()) { return; }
	if (bReady)
		return;
	if (bReady) {
		bReady = false;
	} else {
		bReady = true;
	}
	while (!bReady) bReady = true;
}
