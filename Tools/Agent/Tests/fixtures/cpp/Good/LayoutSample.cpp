#include "LayoutSample.h"

#include "Engine/World.h"

namespace
{
	constexpr float DefaultDelay = 0.5f;
}

ALayoutSample::ALayoutSample()
{
}

void ALayoutSample::BeginPlay()
{
	Super::BeginPlay();

	if (!GetWorld())
	{
		return;
	}
	else if (DefaultDelay > 1.f)
	{
		bReady = 0;
	}
	else
	{
		bReady = 1;
	}

	for (int32 Index = 0; Index < 2; ++Index)
	{
		bReady = 1;
	}
}
