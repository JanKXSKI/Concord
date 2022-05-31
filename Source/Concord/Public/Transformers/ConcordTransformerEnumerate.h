// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerEnumerate.generated.h"

UCLASS()
class CONCORD_API UConcordTransformerEnumerate : public UConcordTransformer
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Enumerate")
    int32 DimensionIndex;

    UConcordTransformerEnumerate()
        : DimensionIndex(0)
        , ActualDimensionIndex(0)
    {}
    
    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, {}, {}, "0"}
        };
    }

    FString GetCategory() const override { return TEXT("Index"); }

    FText GetDisplayName() const override { return INVTEXT("Enumerate"); }
    FText GetTooltip() const override { return INVTEXT("Outputs the indices along a single dimension of the input."); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& InMultiIndex) const override
    {
        return SharedExpressions[InMultiIndex[ActualDimensionIndex]];
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        const FConcordShape& ConnectedShape = GetConnectedTransformers()[0]->GetShape();
        if (!GetActualDimensionIndex(DimensionIndex, ConnectedShape, ActualDimensionIndex, OutErrorMessage)) return {};
        SharedExpressions.Reset();
        for (int32 Index = 0; Index < ConnectedShape[ActualDimensionIndex]; ++Index)
            SharedExpressions.Add(MakeShared<const FConcordValueExpression<int32>>(Index));
        return { ConnectedShape, EConcordValueType::Int };
    }

    int32 ActualDimensionIndex;
    TArray<FConcordSharedExpression> SharedExpressions;
};
