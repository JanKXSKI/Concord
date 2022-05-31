// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerRange.generated.h"

UCLASS()
class CONCORD_API UConcordTransformerRange : public UConcordTransformer
{
    GENERATED_BODY()
public:
    UConcordTransformerRange()
        : DimensionIndex(0)
        , Start(0)
        , Step(1)
        , Last(-1)
        , ActualDimensionIndex(0)
        , ActualStart(0)
        , ActualLast(0)
    {}

    UPROPERTY(EditAnywhere, Category = "Range")
    int32 DimensionIndex;

    UPROPERTY(EditAnywhere, Category = "Range")
    int32 Start;

    UPROPERTY(EditAnywhere, Category = "Range", meta=(ClampMin=1))
    int32 Step;

    UPROPERTY(EditAnywhere, Category = "Range")
    int32 Last;

    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, {}, {}, "0"}
        };
    }

    FString GetCategory() const override { return TEXT("Index"); }

    FText GetDisplayName() const override { return INVTEXT("Range"); }
    FText GetTooltip() const override { return INVTEXT("Statically gets a range of the input."); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& InMultiIndex) const override
    {
        FConcordMultiIndex RangedMultiIndex = InMultiIndex;
        RangedMultiIndex[ActualDimensionIndex] = ActualStart + RangedMultiIndex[ActualDimensionIndex] * Step;
        return GetConnectedTransformers()[0]->GetExpression(RangedMultiIndex);
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        const FConcordShape& ConnectedShape = GetConnectedTransformers()[0]->GetShape();
        if (!GetActualDimensionIndex(DimensionIndex, ConnectedShape, ActualDimensionIndex, OutErrorMessage)) return {};
        if (!GetActualIndex(Start, ConnectedShape[ActualDimensionIndex], ActualStart, OutErrorMessage)) return {};
        if (!GetActualIndex(Last, ConnectedShape[ActualDimensionIndex], ActualLast, OutErrorMessage)) return {};
        ActualLast = FMath::Min(ActualLast, ConnectedShape[ActualDimensionIndex] - 1);
        if (ActualStart > ActualLast) { OutErrorMessage = TEXT("Start exceeds Last."); return {}; }
        TArray<int32> RangeShape = ConnectedShape;
        RangeShape[ActualDimensionIndex] = (ActualLast - ActualStart + Step) / Step;
        return { RangeShape, GetConnectedTransformers()[0]->GetType() };
    }

    int32 ActualDimensionIndex, ActualStart, ActualLast;
};
