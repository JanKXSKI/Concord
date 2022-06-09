// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerReverse.generated.h"

UCLASS()
class CONCORD_API UConcordTransformerReverse : public UConcordTransformer
{
    GENERATED_BODY()
public:
    UConcordTransformerReverse()
        : DimensionIndex(-1)
        , ActualDimensionIndex(0)
    {}

    UPROPERTY(EditAnywhere, Category = "Reverse")
    int32 DimensionIndex;

    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, {}, {}, "0"}
        };
    }

    FString GetCategory() const override { return TEXT("Sequence"); }

    FText GetDisplayName() const override { return INVTEXT("Reverse"); }
    FText GetTooltip() const override { return INVTEXT("Reverses the input in the chosen dimension."); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& MultiIndex) const override
    {
        FConcordMultiIndex ReversedMultiIndex = MultiIndex;
        ReversedMultiIndex[ActualDimensionIndex] = GetShape()[ActualDimensionIndex] - 1 - MultiIndex[ActualDimensionIndex];
        return GetConnectedTransformers()[0]->GetExpression(ReversedMultiIndex);
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        if (!GetActualDimensionIndex(DimensionIndex, GetConnectedTransformers()[0]->GetShape(), ActualDimensionIndex, OutErrorMessage)) return {};
        return { GetConnectedTransformers()[0]->GetShape(), GetConnectedTransformers()[0]->GetType() };
    }

    int32 ActualDimensionIndex;
};
