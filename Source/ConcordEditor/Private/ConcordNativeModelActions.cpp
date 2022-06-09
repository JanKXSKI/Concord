// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "ConcordNativeModelActions.h"
#include "ConcordNativeModel.h"

FText FConcordNativeModelActions::GetName() const
{
    return INVTEXT("Native Concord Model");
}

uint32 FConcordNativeModelActions::GetCategories()
{
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    return AssetTools.FindAdvancedAssetCategory(FName(TEXT("Concord")));
}

FColor FConcordNativeModelActions::GetTypeColor() const
{
     return FColor::Purple;
}

FText FConcordNativeModelActions::GetAssetDescription(const FAssetData &AssetData) const
{
    return INVTEXT("A native Concord model whose implementation is compiled from auto-generated C++ code.");
}

UClass* FConcordNativeModelActions::GetSupportedClass() const
{
    return UConcordNativeModel::StaticClass();
}
