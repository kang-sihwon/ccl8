#include "CCLMassAgentBridge.h"

#include "MassEntityManager.h"

FMassEntityHandle CCLMassAgentBridge::Enter(FMassEntityManager& Manager, FCCLAgentStore& Store, FGuid Id)
{
	const auto Handle = Store.GetHandle(Id);
	const auto* Record = Store.Find(Handle);
	if (!Record)
	{
		return {};
	}
	const auto Lease = Store.Acquire(Handle, FGuid::NewGuid());
	if (!Lease.Writer.IsValid())
	{
		return {};
	}
	const TArray<const UScriptStruct*> Types = {FCCLMassAgentFragment::StaticStruct()};
	const auto Entity = Manager.CreateEntity(Manager.CreateArchetype(Types));
	auto& Fragment = Manager.GetFragmentDataChecked<FCCLMassAgentFragment>(Entity);
	Fragment.Intent = Record->Intent;
	Fragment.Lease = Lease;
	return Entity;
}

FCCLDecisionResult CCLMassAgentBridge::Decide(FMassEntityManager& Manager, FMassEntityHandle Entity, const FCCLDecisionInput& Input)
{
	if (!Manager.IsEntityValid(Entity))
	{
		return {};
	}
	const auto Result = CCLDecision::Evaluate(Input);
	if (Result.bSelected)
	{
		Manager.GetFragmentDataChecked<FCCLMassAgentFragment>(Entity).Intent = Result.Intent;
	}
	return Result;
}

bool CCLMassAgentBridge::Leave(FMassEntityManager& Manager, FMassEntityHandle Entity, FCCLAgentStore& Store,
	const FCCLFeatureRegistry& Registry, FString& Error)
{
	if (!Manager.IsEntityValid(Entity))
	{
		return false;
	}
	const auto& Fragment = Manager.GetFragmentDataChecked<FCCLMassAgentFragment>(Entity);
	const auto* Current = Store.Find(Fragment.Lease.Handle);
	if (!Current)
	{
		return false;
	}
	auto Updated = *Current;
	Updated.Intent = Fragment.Intent;
	if (!Store.Commit(Fragment.Lease, MoveTemp(Updated), Registry, Error))
	{
		// Retain the Entity and its pending result for explicit recovery.
		return false;
	}
	Store.Release(Fragment.Lease);
	Manager.DestroyEntity(Entity);
	return true;
}
