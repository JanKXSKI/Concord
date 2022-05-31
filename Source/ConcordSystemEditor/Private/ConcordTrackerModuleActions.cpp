// Copyright Jan Klimaschewski. All Rights Reserved.

#include "ConcordTrackerModuleActions.h"
#include "ConcordTrackerModule.h"

FText FConcordTrackerModuleActions::GetName() const
{
    return INVTEXT("Concord Tracker Module");
}

uint32 FConcordTrackerModuleActions::GetCategories()
{
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    return AssetTools.FindAdvancedAssetCategory(FName(TEXT("Concord")));
}

FColor FConcordTrackerModuleActions::GetTypeColor() const
{
    return FColor::Purple;
}

FText FConcordTrackerModuleActions::GetAssetDescription(const FAssetData &AssetData) const
{
    return INVTEXT("A Tracker Module to play.");
}

UClass* FConcordTrackerModuleActions::GetSupportedClass() const
{
    return UConcordTrackerModule::StaticClass();
}

bool FConcordTrackerModuleActions::IsImportedAsset() const
{
    return true;
}

void FConcordTrackerModuleActions::GetResolvedSourceFilePaths(const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths) const
{
    for (UObject* Asset : TypeAssets)
    {
        const UConcordTrackerModule* TrackerModule = Cast<UConcordTrackerModule>(Asset);
        if (!TrackerModule || !TrackerModule->AssetImportData) continue;
        OutSourceFilePaths.Add(TrackerModule->AssetImportData->GetFirstFilename());
    }
}
