// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerGet.generated.h"

UCLASS()
class CONCORD_API UConcordTransformerGet : public UConcordTransformer
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Get")
    int32 DimensionIndex;

    UPROPERTY(EditAnywhere, Category = "Get")
    int32 Index;

    UConcordTransformerGet()
        : DimensionIndex(0)
        , Index(0)
        , ActualDimensionIndex(0)
        , ActualIndex(0)
    {}

    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, {}, {}, "0"},
        };
    }

    FString GetCategory() const override { return TEXT("Index"); }

    FText GetDisplayName() const override { return INVTEXT("Get"); }
    FText GetTooltip() const override { return INVTEXT("Statically gets part of the input depending on the DimensionIndex and Index."); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& InMultiIndex) const override
    {
        FConcordMultiIndex GotMultiIndex = InMultiIndex;
        if (GetConnectedTransformers()[0]->GetShape().Num() == 1)
            GotMultiIndex[0] = ActualIndex;
        else
            GotMultiIndex.Insert(ActualIndex, ActualDimensionIndex);
        return GetConnectedTransformers()[0]->GetExpression(GotMultiIndex);
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        TArray<int32> GotShape = GetConnectedTransformers()[0]->GetShape();
        if (!GetActualDimensionIndex(DimensionIndex, GotShape, ActualDimensionIndex, OutErrorMessage)) return {};
        if (!GetActualIndex(Index, GotShape[ActualDimensionIndex], ActualIndex, OutErrorMessage)) return {};
        GotShape.RemoveAt(ActualDimensionIndex, 1, false);
        if (GotShape.IsEmpty()) GotShape = { 1 };
        return { GotShape, GetConnectedTransformers()[0]->GetType() };
    }

    int32 ActualDimensionIndex;
    int32 ActualIndex;
};
