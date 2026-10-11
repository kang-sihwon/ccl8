#include "CCLEnvironmentFXLibrary.h"

#if WITH_EDITOR
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraSystem.h"
#include "Stateless/NiagaraStatelessEmitter.h"
#include "Stateless/Modules/NiagaraStatelessModule_AddVelocity.h"
#include "Stateless/Modules/NiagaraStatelessModule_Drag.h"
#include "Stateless/Modules/NiagaraStatelessModule_GravityForce.h"
#include "Stateless/Modules/NiagaraStatelessModule_InitializeParticle.h"
#include "Stateless/Modules/NiagaraStatelessModule_ShapeLocation.h"
#include "UObject/UnrealType.h"

namespace
{
	// Edits a reflected struct the Details panel can edit. A renamed or retyped property fails the authoring step
	// instead of silently leaving the old value after an engine update.
	template <typename T>
	T* EditableStruct(UObject* Object, const TCHAR* Name, FName ExpectedStruct)
	{
		const auto* Property = FindFProperty<FStructProperty>(Object->GetClass(), Name);
		if (!Property || !Property->Struct || Property->Struct->GetFName() != ExpectedStruct)
		{
			UE_LOG(LogTemp, Error, TEXT("CCL_FX %s.%s is missing or has an unexpected type."), *Object->GetClass()->GetName(), Name);
			return nullptr;
		}

		return Property->ContainerPtrToValuePtr<T>(Object);
	}
}
#endif

// 내 클래스 함수

bool UCCLEnvironmentFXLibrary::ConfigurePrecipitationSystem(UNiagaraSystem* System, bool bSnow)
{
#if WITH_EDITOR
	if (!System || System->GetEmitterHandles().Num() != 1)
	{
		return false;
	}

	auto* E = System->GetEmitterHandles()[0].GetStatelessEmitter();
	if (!E)
	{
		return false;
	}

	auto* Init = Cast<UNiagaraStatelessModule_InitializeParticle>(E->GetModule(UNiagaraStatelessModule_InitializeParticle::StaticClass()));
	auto* Shape = Cast<UNiagaraStatelessModule_ShapeLocation>(E->GetModule(UNiagaraStatelessModule_ShapeLocation::StaticClass()));
	auto* Velocity = Cast<UNiagaraStatelessModule_AddVelocity>(E->GetModule(UNiagaraStatelessModule_AddVelocity::StaticClass()));
	if (!Init || !Shape || !Velocity || E->GetNumSpawnInfos() < 1)
	{
		return false;
	}

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
	auto* Bounds = EditableStruct<FBox>(E, TEXT("FixedBounds"), NAME_Box);
	if (!Bounds)
	{
		return false;
	}

	*Bounds = FBox(FVector(-150., -150., -300.), FVector(150., 150., 50.));
	auto* State = EditableStruct<FNiagaraEmitterStateData>(E, TEXT("EmitterState"), FNiagaraEmitterStateData::StaticStruct()->GetFName());
	if (!State)
	{
		return false;
	}

	State->LoopBehavior = ENiagaraLoopBehavior::Infinite;
	// Spawn rates are sampled per loop; 2.7s snow lifetime must fit the 8 live spawn records.
	State->LoopDuration = FNiagaraDistributionRangeFloat(bSnow ? 1.f : 0.25f);
	State->bLoopDelayEnabled = false;

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

bool UCCLEnvironmentFXLibrary::ConfigureFireSystem(UNiagaraSystem* System, bool bSmoke, UMaterialInterface* SpriteMaterial)
{
#if WITH_EDITOR
	if (!SpriteMaterial || !ConfigurePrecipitationSystem(System, false))
	{
		return false;
	}

	auto* E = System->GetEmitterHandles()[0].GetStatelessEmitter();
	auto* Init = Cast<UNiagaraStatelessModule_InitializeParticle>(E->GetModule(UNiagaraStatelessModule_InitializeParticle::StaticClass()));
	auto* Shape = Cast<UNiagaraStatelessModule_ShapeLocation>(E->GetModule(UNiagaraStatelessModule_ShapeLocation::StaticClass()));
	Init->LifetimeDistribution = FNiagaraDistributionRangeFloat(bSmoke ? 4.f : 0.8f);
	Init->SpriteSizeDistribution.InitConstant(bSmoke ? FVector2f(90.f, 90.f) : FVector2f(22.f, 45.f));
	Init->ColorDistribution.InitConstant(bSmoke ? FLinearColor(0.08f, 0.075f, 0.07f, 0.3f) : FLinearColor(6.f, 1.2f, 0.05f, 0.8f));
	Shape->PlaneSize.InitConstant(bSmoke ? FVector2f(250.f, 250.f) : FVector2f(35.f, 35.f));
	for (auto* Renderer : E->GetRenderers())
	{
		if (auto* Sprite = Cast<UNiagaraSpriteRendererProperties>(Renderer))
		{
			Sprite->Material = SpriteMaterial;
		}
	}

	auto* Bounds = EditableStruct<FBox>(E, TEXT("FixedBounds"), NAME_Box);
	if (!Bounds)
	{
		return false;
	}

	*Bounds = FBox(FVector(-400., -400., -100.), FVector(400., 400., 600.));
	auto* State = EditableStruct<FNiagaraEmitterStateData>(E, TEXT("EmitterState"), FNiagaraEmitterStateData::StaticStruct()->GetFName());
	if (!State)
	{
		return false;
	}

	State->LoopDuration = FNiagaraDistributionRangeFloat(bSmoke ? 2.f : 1.f);
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
