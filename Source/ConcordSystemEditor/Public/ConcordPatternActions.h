// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class CONCORDSYSTEMEDITOR_API FConcordPatternActions : public FAssetTypeActions_Base
{
public:
    FText GetName() const override;
    uint32 GetCategories() override;
    FColor GetTypeColor() const override;
    FText GetAssetDescription(const FAssetData &AssetData) const override;
    UClass* GetSupportedClass() const override;
    bool IsImportedAsset() const override;
    void GetResolvedSourceFilePaths( const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths ) const override;

    bool HasActions(const TArray<UObject*>& InObjects) const override;
    void GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder) override;
};
