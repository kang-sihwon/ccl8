#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLFrameworkRedirectTest, "CCL.Framework.MigratedTypes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLFrameworkRedirectTest::RunTest(const FString& Parameters)
{
    struct FTypePath { const TCHAR* Old; const TCHAR* Current; };
    const FTypePath Types[] = {
        {TEXT("/Script/CCL.CCLInventoryEntry"), TEXT("/Script/KSIInventory.KSIInventoryEntry")},
        {TEXT("/Script/CCL.CCLInventoryList"), TEXT("/Script/KSIInventory.KSIInventoryList")},
        {TEXT("/Script/CCL.CCLItemFragmentData"), TEXT("/Script/KSIInventory.KSIItemFragmentData")},
        {TEXT("/Script/CCL.CCLScreen"), TEXT("/Script/KSIUI.KSIScreen")},
        {TEXT("/Script/CCL.CCLUIActionRouter"), TEXT("/Script/KSIUI.KSIUIActionRouter")},
        {TEXT("/Script/CCL.CCLUIContext"), TEXT("/Script/KSIUI.KSIUIContext")},
        {TEXT("/Script/CCL.CCLUIPresentationHandle"), TEXT("/Script/KSIUI.KSIUIPresentationHandle")},
        {TEXT("/Script/CCL.CCLUIViewPresentation"), TEXT("/Script/KSIUI.KSIUIViewPresentation")},
        {TEXT("/Script/CCL.CCLUIPresentationDefinition"), TEXT("/Script/KSIUI.KSIUIPresentationDefinition")},
        {TEXT("/Script/CCL.CCLUIPresentationRequest"), TEXT("/Script/KSIUI.KSIUIPresentationRequest")},
        {TEXT("/Script/CCL.CCLUIViewDefinition"), TEXT("/Script/KSIUI.KSIUIViewDefinition")},
        {TEXT("/Script/CCL.CCLUIRegistry"), TEXT("/Script/KSIUI.KSIUIRegistry")},
        {TEXT("/Script/CCL.CCLUIRoot"), TEXT("/Script/KSIUI.KSIUIRoot")},
        {TEXT("/Script/CCL.CCLUIRegistration"), TEXT("/Script/KSIUI.KSIUIRegistration")},
        {TEXT("/Script/CCL.CCLUIOpenView"), TEXT("/Script/KSIUI.KSIUIOpenView")},
        {TEXT("/Script/CCL.CCLUIPendingView"), TEXT("/Script/KSIUI.KSIUIPendingView")},
        {TEXT("/Script/CCL.CCLUISubsystem"), TEXT("/Script/KSIUI.KSIUISubsystem")},
        {TEXT("/Script/CCL.ECCLUIInstancePolicy"), TEXT("/Script/KSIUI.EKSIUIInstancePolicy")},
        {TEXT("/Script/CCL.ECCLUILayerLayout"), TEXT("/Script/KSIUI.EKSIUILayerLayout")},
        {TEXT("/Script/CCL.ECCLUIScope"), TEXT("/Script/KSIUI.EKSIUIScope")},
        {TEXT("/Script/CCL.ECCLUIInputPolicy"), TEXT("/Script/KSIUI.EKSIUIInputPolicy")},
        {TEXT("/Script/CCL.CCLUIViewHandle"), TEXT("/Script/KSIUI.KSIUIViewHandle")},
        {TEXT("/Script/CCL.CCLUIRegistrationHandle"), TEXT("/Script/KSIUI.KSIUIRegistrationHandle")},
        {TEXT("/Script/CCL.CCLUIRequestHandle"), TEXT("/Script/KSIUI.KSIUIRequestHandle")},
        {TEXT("/Script/CCL.CCLUILayerDefinition"), TEXT("/Script/KSIUI.KSIUILayerDefinition")},
        {TEXT("/Script/CCL.CCLUIExtensionDefinition"), TEXT("/Script/KSIUI.KSIUIExtensionDefinition")},
        {TEXT("/Script/CCL.ECCLMapKind"), TEXT("/Script/KSIMap.EKSIMapKind")},
        {TEXT("/Script/CCL.CCLMapMarkerComponent"), TEXT("/Script/KSIMap.KSIMapMarkerComponent")},
        {TEXT("/Script/CCL.CCLMapMarkerView"), TEXT("/Script/KSIMap.KSIMapMarkerView")},
        {TEXT("/Script/CCL.CCLMapTerrain"), TEXT("/Script/KSIMap.KSIMapTerrain")},
        {TEXT("/Script/CCL.CCLMapRegionDefinition"), TEXT("/Script/KSIMap.KSIMapRegionDefinition")},
        {TEXT("/Script/CCL.CCLMapSubsystem"), TEXT("/Script/KSIMap.KSIMapSubsystem")},
        {TEXT("/Script/CCL.CCLCombatAssetLibrary"), TEXT("/Script/CCLEditor.CCLCombatAssetLibrary")},
        {TEXT("/Script/CCL.CCLEnvironmentFXLibrary"), TEXT("/Script/CCLEditor.CCLEnvironmentFXLibrary")},
        {TEXT("/Script/CCL.CCLProgressionAssetLibrary"), TEXT("/Script/CCLEditor.CCLProgressionAssetLibrary")},
    };
    for (const auto& Type : Types)
    {
        FSoftObjectPath LegacyPath(Type.Old);
        TestTrue(Type.Old, LegacyPath.FixupCoreRedirects());
        TestEqual(TEXT("Redirect target path"), LegacyPath.ToString(), FString(Type.Current));
        UObject* Legacy = LegacyPath.TryLoad();
        UObject* Current = FSoftObjectPath(Type.Current).TryLoad();
        TestNotNull(Type.Current, Current);
        TestEqual(Type.Old, Legacy, Current);
        if (const auto* Enum = Cast<UEnum>(Current))
        {
            FString OldPath(Type.Old);
            FString Package, OldName;
            OldPath.Split(TEXT("."), &Package, &OldName);
            for (int32 Index = 0; Index < Enum->NumEnums() - 1; ++Index)
            {
                const FString Value = OldName + TEXT("::") + Enum->GetNameStringByIndex(Index);
                TestEqual(*Value, Enum->GetValueByNameString(Value), Enum->GetValueByIndex(Index));
            }
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLFrameworkAssetLoadTest, "CCL.Framework.ExistingAssets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLFrameworkAssetLoadTest::RunTest(const FString& Parameters)
{
    TArray<FString> Files;
    IFileManager::Get().FindFilesRecursive(Files, *FPaths::ProjectContentDir(), TEXT("*.uasset"), true, false);
    TArray<FString> Maps;
    IFileManager::Get().FindFilesRecursive(Maps, *FPaths::ProjectContentDir(), TEXT("*.umap"), true, false);
    Files.Append(Maps);
    TestTrue(TEXT("Existing content is present"), !Files.IsEmpty());
    for (const FString& File : Files)
    {
        FString Package;
        if (!FPackageName::TryConvertFilenameToLongPackageName(File, Package))
        {
            AddError(FString::Printf(TEXT("Invalid package filename: %s"), *File));
            continue;
        }
        TestNotNull(*Package, LoadPackage(nullptr, *Package, LOAD_None));
    }
    AddInfo(FString::Printf(TEXT("Loaded %d existing packages without resaving"), Files.Num()));
    UClass* Probe = LoadClass<UObject>(nullptr, TEXT("/Game/Tests/UI/BP_UIAsyncProbe.BP_UIAsyncProbe_C"));
    TestNotNull(TEXT("Legacy UI Blueprint parent resolves"), Probe);
    if (Probe)
    {
        TestEqual(TEXT("Blueprint parent is the moved screen"), Probe->GetSuperClass()->GetPathName(), FString(TEXT("/Script/KSIUI.KSIScreen")));
    }
    return true;
}
#endif
