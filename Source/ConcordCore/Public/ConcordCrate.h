// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EditorFramework/AssetImportData.h"
#include "ConcordCrate.generated.h"

USTRUCT(BlueprintType)
struct FConcordIntBlock
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Concord Crate")
    TArray<int32> Values;
};

USTRUCT(BlueprintType)
struct FConcordFloatBlock
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Concord Crate")
    TArray<float> Values;
};

USTRUCT(BlueprintType)
struct FConcordCrateData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Concord Crate")
    TMap<FName, FConcordIntBlock> IntBlocks;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Concord Crate")
    TMap<FName, FConcordFloatBlock> FloatBlocks;
};

UCLASS(BlueprintType)
class CONCORDCORE_API UConcordCrate : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Concord Crate")
    FConcordCrateData CrateData;

#if WITH_EDITORONLY_DATA
    UPROPERTY(VisibleAnywhere, Instanced, Category = "ImportSettings")
    class UAssetImportData* AssetImportData;

    void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override
    {
        Super::GetAssetRegistryTags(OutTags);

        if (AssetImportData)
            OutTags.Add(FAssetRegistryTag(SourceFileTagName(), AssetImportData->GetSourceData().ToJson(), FAssetRegistryTag::TT_Hidden));
    }
#endif
};
