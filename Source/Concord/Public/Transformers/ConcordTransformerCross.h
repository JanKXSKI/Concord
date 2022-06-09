// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerCross.generated.h"

UCLASS()
class CONCORD_API UConcordTransformerCross : public UConcordTransformer
{
    GENERATED_BODY()
public:
    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, {}, {}, "0"},
            { {}, {}, {}, "0"}
        };
    }

    FString GetCategory() const override { return TEXT("Shape"); }

    FText GetDisplayName() const override { return INVTEXT("Cross"); }
    FText GetTooltip() const override { return INVTEXT("Computes the Cartesian product of the inputs."); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& MultiIndex) const override
    {
        FConcordMultiIndex CrossedMultiIndex = MultiIndex;
        const int32 SourceIndex = CrossedMultiIndex.Pop();
        const int32 DimCount0 = GetConnectedTransformers()[0]->GetShape().Num();
        if (SourceIndex == 0) CrossedMultiIndex.SetNum(DimCount0);
        else CrossedMultiIndex.RemoveAt(0, DimCount0, false);
        return GetConnectedTransformers()[SourceIndex]->GetExpression(CrossedMultiIndex);
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        if (GetConnectedTransformers()[0]->GetType() != GetConnectedTransformers()[1]->GetType())
            OutErrorMessage = TEXT("Input types must match.");
        FConcordShape CrossShape = GetConnectedTransformers()[0]->GetShape();
        CrossShape.Append(GetConnectedTransformers()[1]->GetShape());
        CrossShape.Add(2);
        return { MoveTemp(CrossShape), GetConnectedTransformers()[0]->GetType() };
    }
};
