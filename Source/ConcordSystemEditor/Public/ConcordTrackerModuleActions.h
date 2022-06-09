// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class CONCORDSYSTEMEDITOR_API FConcordTrackerModuleActions : public FAssetTypeActions_Base
{
public:
    FText GetName() const override;
    uint32 GetCategories() override;
    FColor GetTypeColor() const override;
    FText GetAssetDescription(const FAssetData &AssetData) const override;
    UClass* GetSupportedClass() const override;
    bool IsImportedAsset() const override;
    void GetResolvedSourceFilePaths( const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths ) const override;
};
