// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "ConcordCrateActions.h"
#include "ConcordCrate.h"

FText FConcordCrateActions::GetName() const
{
    return INVTEXT("Concord Crate");
}

uint32 FConcordCrateActions::GetCategories()
{
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    return AssetTools.FindAdvancedAssetCategory(FName(TEXT("Concord")));
}

FColor FConcordCrateActions::GetTypeColor() const
{
     return FColor::Purple;
}

FText FConcordCrateActions::GetAssetDescription(const FAssetData &AssetData) const
{
    return INVTEXT("A collection of blocks with values to be observed or parameters to be set in a Concord model.");
}

UClass* FConcordCrateActions::GetSupportedClass() const
{
    return UConcordCrate::StaticClass();
}

bool FConcordCrateActions::IsImportedAsset() const
{
    return true;
}

void FConcordCrateActions::GetResolvedSourceFilePaths(const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths) const
{
    for (UObject* Asset : TypeAssets)
    {
        const UConcordCrate* Crate = Cast<UConcordCrate>(Asset);
        if (!Crate || !Crate->AssetImportData) continue;
        OutSourceFilePaths.Add(Crate->AssetImportData->GetFirstFilename());
    }
}
