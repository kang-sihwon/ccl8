#include "CCLSnowAuthoring.h"

#if WITH_EDITOR
#include "CCLSnowAnimInstance.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimBlueprintGeneratedClass.h"
#include "Animation/Skeleton.h"
#include "AnimGraphNode_LinkedInputPose.h"
#include "AnimGraphNode_Root.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphSchema.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/Package.h"
#endif

bool UCCLSnowAuthoring::BuildPostProcessAsset()
{
#if WITH_EDITOR
	const TCHAR* Path = TEXT("/Game/Environment/Animation/ABP_SnowPostProcess");
	auto* BP = LoadObject<UAnimBlueprint>(nullptr, TEXT("/Game/Environment/Animation/ABP_SnowPostProcess.ABP_SnowPostProcess"));
	if (!BP)
	{
		auto* Package = CreatePackage(Path);
		BP = Cast<UAnimBlueprint>(FKismetEditorUtilities::CreateBlueprint(UCCLSnowAnimInstance::StaticClass(), Package,
			TEXT("ABP_SnowPostProcess"), BPTYPE_Normal, UAnimBlueprint::StaticClass(), UAnimBlueprintGeneratedClass::StaticClass()));
		if (!BP)
		{
			return false;
		}
		FAssetRegistryModule::AssetCreated(BP);
	}
	BP->TargetSkeleton = LoadObject<USkeleton>(nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SK_Mannequin.SK_Mannequin"));
	if (!BP->TargetSkeleton || BP->ParentClass != UCCLSnowAnimInstance::StaticClass())
	{
		return false;
	}
	for (UEdGraph* Graph : BP->FunctionGraphs)
	{
		UAnimGraphNode_Root* Root = nullptr;
		UAnimGraphNode_LinkedInputPose* Input = nullptr;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (auto* Candidate = Cast<UAnimGraphNode_Root>(Node))
			{
				Root = Candidate;
			}
			if (auto* Candidate = Cast<UAnimGraphNode_LinkedInputPose>(Node))
			{
				Input = Candidate;
			}
		}
		if (!Root)
		{
			continue;
		}
		if (!Input)
		{
			FGraphNodeCreator<UAnimGraphNode_LinkedInputPose> Creator(*Graph);
			Input = Creator.CreateNode();
			Input->Node.Name = FAnimNode_LinkedInputPose::DefaultInputPoseName;
			Input->NodePosX = -300;
			Creator.Finalize();
		}
		bool bConnected = false;
		for (auto* Out : Input->Pins)
		{
			for (auto* In : Root->Pins)
			{
				if (Out->Direction == EGPD_Output && In->Direction == EGPD_Input)
				{
					bConnected |= Out->LinkedTo.Contains(In) || Graph->GetSchema()->TryCreateConnection(Out, In);
				}
			}
		}
		if (!bConnected)
		{
			return false;
		}
		FKismetEditorUtilities::CompileBlueprint(BP);
		BP->MarkPackageDirty();
		return BP->Status == BS_UpToDate && BP->GeneratedClass != nullptr;
	}
#endif
	return false;
}
