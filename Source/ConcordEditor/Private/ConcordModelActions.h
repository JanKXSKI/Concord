// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class FConcordModelActions : public FAssetTypeActions_Base
{
public:
    FText GetName() const override;
    uint32 GetCategories() override;
    FColor GetTypeColor() const override;
    FText GetAssetDescription(const FAssetData &AssetData) const override;
    UClass* GetSupportedClass() const override;
    void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
};
