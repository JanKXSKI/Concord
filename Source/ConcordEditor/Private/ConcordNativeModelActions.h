// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class FConcordNativeModelActions : public FAssetTypeActions_Base
{
public:
    FText GetName() const override;
    uint32 GetCategories() override;
    FColor GetTypeColor() const override;
    FText GetAssetDescription(const FAssetData &AssetData) const override;
    UClass* GetSupportedClass() const override;
};
