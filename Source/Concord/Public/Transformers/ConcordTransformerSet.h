// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerSet.generated.h"

UCLASS()
class CONCORD_API UConcordTransformerSet : public UConcordTransformer
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Set")
    int32 DimensionIndex;

    UPROPERTY(EditAnywhere, Category = "Set")
    int32 Index;

    UConcordTransformerSet()
        : DimensionIndex(0)
        , Index(0)
        , ActualDimensionIndex(0)
        , ActualIndex(0)
    {}

    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, {}, INVTEXT("Target"), "0"},
            { {}, {}, INVTEXT("Source"), "0"}
        };
    }

    FString GetCategory() const override { return TEXT("Index"); }

    FText GetDisplayName() const override { return INVTEXT("Set"); }
    FText GetTooltip() const override { return INVTEXT("Statically sets part of the target input to the source input depending on the DimensionIndex and Index."); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& InMultiIndex) const override
    {
        const EConcordValueType SumType = GetSumType();
        FConcordMultiIndex SetMultiIndex = InMultiIndex;
        if (SetMultiIndex[ActualDimensionIndex] == ActualIndex)
        {
            SetMultiIndex.RemoveAt(0, ActualDimensionIndex + 1, false);
            if (SetMultiIndex.IsEmpty()) SetMultiIndex = { 0 };
            ConcordShape::Unbroadcast(SetMultiIndex, GetConnectedTransformers()[1]->GetShape());
            return GetConnectedExpression(GetConnectedTransformers()[1], SetMultiIndex, SumType);
        }
        ConcordShape::Unbroadcast(SetMultiIndex, GetConnectedTransformers()[0]->GetShape());
        return GetConnectedExpression(GetConnectedTransformers()[0], SetMultiIndex, SumType);
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        TArray<int32> SetShape = GetConnectedTransformers()[0]->GetShape();
        if (!GetActualDimensionIndex(DimensionIndex, SetShape, ActualDimensionIndex, OutErrorMessage)) return {};
        if (!GetActualIndex(Index, SetShape[ActualDimensionIndex], ActualIndex, OutErrorMessage)) return {};
        TArray<int32> RefShape = SetShape;
        RefShape.RemoveAt(0, ActualDimensionIndex + 1, false);
        TOptional<FConcordShape> OptionalBroadcastedShape = ConcordShape::Broadcast({ RefShape, GetConnectedTransformers()[1]->GetShape() });
        if (!OptionalBroadcastedShape) { OutErrorMessage = TEXT("Input shapes could not be broadcasted together."); return {}; }
        SetShape.SetNum(ActualDimensionIndex + 1);
        const FConcordShape& BroadcastedShape = OptionalBroadcastedShape.GetValue();
        if (!RefShape.IsEmpty() || BroadcastedShape.Num() > 1 || BroadcastedShape[0] > 1) SetShape.Append(BroadcastedShape);

        return { SetShape, GetSumType() };
    }

    int32 ActualDimensionIndex;
    int32 ActualIndex;
};
