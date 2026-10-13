#include "CCLTestResourceLibrary.h"

#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Misc/PackageName.h"
#include "Serialization/ArchiveReplaceObjectRef.h"
#include "UObject/UObjectHash.h"
#include "UObject/Package.h"

bool UCCLTestResourceLibrary::CopyResources(const TMap<FString, FString>& Packages)
{
	for (const auto& Pair : Packages)
	{
		if (!Pair.Key.StartsWith(TEXT("/Game/Fab/")) || !Pair.Value.StartsWith(TEXT("/Game/TestResources/")) || FPackageName::DoesPackageExist(Pair.Value)) { return false; }
	}
	return FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().AdvancedCopyPackages(Packages, true, false);
}

bool UCCLTestResourceLibrary::RepointResources(UObject* Object, const TMap<FString, FString>& Packages)
{
	if (!Object) { return false; }
	TMap<UObject*, UObject*> Replacements;
	TMap<FSoftObjectPath, FSoftObjectPath> SoftPaths;
	for (const auto& Pair : Packages)
	{
		const FString Old = Pair.Key + TEXT(".") + FPackageName::GetShortName(Pair.Key);
		const FString New = Pair.Value + TEXT(".") + FPackageName::GetShortName(Pair.Value);
		auto* Source = LoadObject<UObject>(nullptr, *Old);
		auto* Target = LoadObject<UObject>(nullptr, *New);
		if (!Source || !Target) { return false; }
		Replacements.Add(Source, Target);
		SoftPaths.Add(FSoftObjectPath(Old), FSoftObjectPath(New));
	}
	Object->Modify();
	FArchiveReplaceObjectRef<UObject> Replace(Object, Replacements, EArchiveReplaceObjectFlags::IgnoreOuterRef | EArchiveReplaceObjectFlags::IgnoreArchetypeRef);
	FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().RenameReferencingSoftObjectPaths({Object->GetPackage()}, SoftPaths);
	Object->MarkPackageDirty();
	return true;
}
