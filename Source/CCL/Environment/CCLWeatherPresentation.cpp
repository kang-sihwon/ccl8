#include "CCLWeatherPresentation.h"

#include "CCLSurfaceReplication.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Components/VolumetricCloudComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/PlayerController.h"
#if WITH_EDITOR
#include "UObject/UnrealType.h"
#include "Stateless/NiagaraStatelessEmitter.h"
#include "Stateless/Modules/NiagaraStatelessModule_InitializeParticle.h"
#include "Stateless/Modules/NiagaraStatelessModule_ShapeLocation.h"
#include "Stateless/Modules/NiagaraStatelessModule_AddVelocity.h"
#include "Stateless/Modules/NiagaraStatelessModule_GravityForce.h"
#include "Stateless/Modules/NiagaraStatelessModule_Drag.h"
#endif

ACCLWeatherPresentation::ACCLWeatherPresentation()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Clouds = CreateDefaultSubobject<UVolumetricCloudComponent>(TEXT("Clouds"));
	Clouds->SetupAttachment(RootComponent);
	Clouds->SetVisibility(false);
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.2f;
}

float ACCLWeatherPresentation::GetCloudFraction(const UWorld* World)
{
	const auto* Surface = UCCLSurfaceReplication::View(World);
	const auto* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (!Surface || !PC) { return 0.f; }
	FVector Camera;
	FRotator Rotation;
	PC->GetPlayerViewPoint(Camera, Rotation);
	double Nearest = TNumericLimits<double>::Max();
	float Result = 0.f;
	for (const auto& Pair : Surface->GetRegions())
	{
		const auto& G = Pair.Value;
		const double Distance = FVector::DistSquared2D(Camera / 100., G.OriginMeters);
		if (G.bDynamicShelter && Distance < Nearest)
		{
			Nearest = Distance;
			Result = float(FMath::Clamp(G.Weather.CloudMeters / 0.003, 0., 1.));
		}
	}
	return Result;
}

void ACCLWeatherPresentation::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (GetNetMode() == NM_DedicatedServer || !RainSystem || !SnowSystem)
	{
		return;
	}
	if (CloudMaterial && !CloudInstance)
	{
		CloudInstance = UMaterialInstanceDynamic::Create(CloudMaterial, this);
		Clouds->SetMaterial(CloudInstance);
	}
	if (CloudInstance)
	{
		const float Fraction = GetCloudFraction(GetWorld());
		CloudInstance->SetScalarParameterValue(TEXT("Cloud_GlobalCoverage"), Fraction);
		Clouds->SetVisibility(Fraction > 0.001f);
	}
	const auto* Surface = UCCLSurfaceReplication::View(GetWorld());
	const auto* PC = GetWorld()->GetFirstPlayerController();
	if (!Surface || !PC)
	{
		return;
	}
	FVector Camera;
	FRotator Rotation;
	PC->GetPlayerViewPoint(Camera, Rotation);
	int32 At = 0;
	for (const auto& Pair : Surface->GetRegions())
	{
		const auto& G = Pair.Value;
		if (!G.bDynamicShelter || FVector::DistSquared2D(Camera / 100., G.OriginMeters) > FMath::Square(40.))
		{
			continue;
		}
		// One bounded emitter per 2m tile: exposed tiles stop at the authoritative bed.
		for (int32 Y = 0; Y < G.Size.Y; Y += 4)
		{
			for (int32 X = 0; X < G.Size.X; X += 4)
			{
				while (Rain.Num() <= At)
				{
					auto Make = [&](UNiagaraSystem* Asset)
					{
						auto* C = NewObject<UNiagaraComponent>(this, NAME_None, RF_Transient);
						C->SetupAttachment(RootComponent);
						C->SetAutoActivate(false);
						C->SetAsset(Asset);
						C->RegisterComponent();
						return C;
					};
					Rain.Add(Make(RainSystem));
					Snow.Add(Make(SnowSystem));
				}
				const int32 Index = X + Y * G.Size.X;
				const auto& Cell = G.Cells[Index];
				const FVector Position((G.OriginMeters.X + (X + 2.) * G.SpacingMeters) * 100.,
					(G.OriginMeters.Y + (Y + 2.) * G.SpacingMeters) * 100., (Cell.BedMeters + 2.8) * 100.);
				auto Update = [&](UNiagaraComponent* C, double Rate, bool bSnow)
				{
					const float Spawn = float(FMath::Clamp(Rate * 3600000. * 3. * Cell.RainExposure, 0., 240.));
					C->SetWorldLocation(Position);
					C->SetVariableFloat(TEXT("User.SpawnRate"), Spawn);
					C->SetVariableVec3(TEXT("User.Velocity"), FVector(0., 0., bSnow ? -100. : -800.));
					if (Spawn > 0.01f && !C->IsActive()) { C->Activate(true); }
					if (Spawn <= 0.01f && C->IsActive()) { C->DeactivateImmediate(); }
				};
				Update(Rain[At], G.Forcing.RainMetersPerWorldSecond, false);
				Update(Snow[At], G.Forcing.SnowMetersPerWorldSecond, true);
				++At;
			}
		}
	}
	for (int32 I = At; I < Rain.Num(); ++I)
	{
		Rain[I]->DeactivateImmediate();
		Snow[I]->DeactivateImmediate();
	}
}

bool ACCLWeatherPresentation::ConfigurePrecipitationSystem(UNiagaraSystem* System, bool bSnow)
{
#if WITH_EDITOR
	if (!System || System->GetEmitterHandles().Num() != 1) { return false; }
	auto* E = System->GetEmitterHandles()[0].GetStatelessEmitter();
	if (!E) { return false; }
	auto* Init = Cast<UNiagaraStatelessModule_InitializeParticle>(E->GetModule(UNiagaraStatelessModule_InitializeParticle::StaticClass()));
	auto* Shape = Cast<UNiagaraStatelessModule_ShapeLocation>(E->GetModule(UNiagaraStatelessModule_ShapeLocation::StaticClass()));
	auto* Velocity = Cast<UNiagaraStatelessModule_AddVelocity>(E->GetModule(UNiagaraStatelessModule_AddVelocity::StaticClass()));
	if (!Init || !Shape || !Velocity || E->GetNumSpawnInfos() < 1) { return false; }
	Init->LifetimeDistribution = FNiagaraDistributionRangeFloat(bSnow ? 2.7f : 0.3375f);
	Init->SpriteSizeDistribution.InitConstant(bSnow ? FVector2f(3.f, 3.f) : FVector2f(0.8f, 12.f));
	Shape->SetIsModuleEnabled(true);
	Shape->ShapePrimitive = ENSM_ShapePrimitive::Plane;
	Shape->PlaneSize.InitConstant(FVector2f(190.f, 190.f));
	Velocity->SetIsModuleEnabled(true);
	Velocity->VelocityType = ENSM_VelocityType::Linear;
	Velocity->LinearVelocityDistribution.Mode = ENiagaraDistributionMode::Binding;
	Velocity->LinearVelocityDistribution.ParameterBinding = FNiagaraVariableBase(FNiagaraTypeDefinition::GetVec3Def(), TEXT("User.Velocity"));
	for (const auto& Module : E->GetModules())
	{
		if (Module->IsA<UNiagaraStatelessModule_GravityForce>() || Module->IsA<UNiagaraStatelessModule_Drag>())
		{
			Module->SetIsModuleEnabled(false);
		}
	}
	for (int32 I = 0; I < E->GetNumSpawnInfos(); ++I)
	{
		auto* Spawn = E->GetSpawnInfoByIndex(I);
		Spawn->bEnabled = I == 0;
		Spawn->Type = ENiagaraStatelessSpawnInfoType::Rate;
		Spawn->Rate.Mode = ENiagaraDistributionMode::Binding;
		Spawn->Rate.ParameterBinding = FNiagaraVariableBase(FNiagaraTypeDefinition::GetFloatDef(), TEXT("User.SpawnRate"));
	}
	System->GetExposedParameters().SetParameterValue<float>(60.f, FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("User.SpawnRate")), true);
	System->GetExposedParameters().SetParameterValue<FVector3f>(FVector3f(0, 0, bSnow ? -100.f : -800.f), FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("User.Velocity")), true);
	if (auto* Property = FindFProperty<FStructProperty>(E->GetClass(), TEXT("FixedBounds")))
	{
		*Property->ContainerPtrToValuePtr<FBox>(E) = FBox(FVector(-150., -150., -300.), FVector(150., 150., 50.));
	}
	if (auto* Property = FindFProperty<FStructProperty>(E->GetClass(), TEXT("EmitterState")))
	{
		auto* State = Property->ContainerPtrToValuePtr<FNiagaraEmitterStateData>(E);
		State->LoopBehavior = ENiagaraLoopBehavior::Infinite;
		// Spawn rates are sampled per loop; 2.7s snow lifetime must fit the 8 live spawn records.
		State->LoopDuration = FNiagaraDistributionRangeFloat(bSnow ? 1.f : 0.25f);
		State->bLoopDelayEnabled = false;
	}
	E->PostEditChange();
	System->PostEditChange();
	System->RequestCompile(true);
	System->WaitForCompilationComplete(true, false);
	System->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}
