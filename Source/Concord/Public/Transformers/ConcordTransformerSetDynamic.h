// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerSetDynamic.generated.h"

class FConcordSetExpression : public FConcordComputingExpression
{
public:
    FConcordSetExpression(TArray<FConcordSharedExpression>&& InSourceExpressions, int32 InIndex)
        : FConcordComputingExpression(MoveTemp(InSourceExpressions))
        , Index(InIndex)
    {}
    FConcordValue ComputeValue(const FConcordExpressionContext<float>& Context) const override;
#if WITH_EDITOR
    FString ToString() const override;
#endif
private:
    const int32 Index;
};

UCLASS()
class CONCORD_API UConcordTransformerSetDynamic : public UConcordTransformer
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Set (Dynamic)")
    int32 DimensionIndex;

    UConcordTransformerSetDynamic()
        : DimensionIndex(0)
        , ActualDimensionIndex(0)
    {}

    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, {}, INVTEXT("Target"), "0"},
            { {}, EConcordValueType::Int, INVTEXT("Index"), "0"},
            { {}, {}, INVTEXT("Source"), "0"}
        };
    }

    FString GetCategory() const override { return TEXT("Index"); }

    FText GetDisplayName() const override { return INVTEXT("Set (Dynamic)"); }
    FText GetTooltip() const override { return INVTEXT("Dynamically sets part of the Target input to the Source input depending on the Index and the DimensionIndex."); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& InMultiIndex) const override
    {
        const EConcordValueType SumType = GetSumType();
        TArray<FConcordSharedExpression> SourceExpressions;

        FConcordMultiIndex TargetMultiIndex = InMultiIndex;
        ConcordShape::Unbroadcast(TargetMultiIndex, GetConnectedTransformers()[0]->GetShape());
        SourceExpressions.Add(GetConnectedExpression(GetConnectedTransformers()[0], TargetMultiIndex, SumType));

        FConcordMultiIndex ElementMultiIndex = InMultiIndex;
        ElementMultiIndex.RemoveAt(0, ActualDimensionIndex + 1, false);

        const FConcordShape& IndexShape = GetConnectedTransformers()[1]->GetShape();
        ConcordShape::FShapeIterator IndexIterator(IndexShape);
        while (IndexIterator.HasNext())
        {
            FConcordMultiIndex SourceMultiIndex = IndexIterator.Next();
            SourceExpressions.Add(GetConnectedTransformers()[1]->GetExpression(SourceMultiIndex));
            if (IndexShape.Num() == 1 && IndexShape[0] == 1) SourceMultiIndex.Reset();
            SourceMultiIndex.Append(ElementMultiIndex);
            if (SourceMultiIndex.IsEmpty()) SourceMultiIndex = { 1 };
            ConcordShape::Unbroadcast(SourceMultiIndex, GetConnectedTransformers()[2]->GetShape());
            SourceExpressions.Add(GetConnectedExpression(GetConnectedTransformers()[2], SourceMultiIndex, SumType));
        }

        return MakeShared<const FConcordSetExpression>(MoveTemp(SourceExpressions), InMultiIndex[ActualDimensionIndex]);
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        const auto& TargetShape = GetConnectedTransformers()[0]->GetShape();
        if (!GetActualDimensionIndex(DimensionIndex, TargetShape, ActualDimensionIndex, OutErrorMessage)) return {};
        FConcordShape TargetElementShape = TargetShape;
        TargetElementShape.RemoveAt(0, ActualDimensionIndex + 1, false);

        FConcordShape IndexShape = GetConnectedTransformers()[1]->GetShape();
        if (IndexShape.Num() == 1 && IndexShape[0] == 1) IndexShape.Reset();

        const auto& SourceShape = GetConnectedTransformers()[2]->GetShape();
        FConcordShape SourcesShape = SourceShape;
        while (SourcesShape.Num() > IndexShape.Num()) SourcesShape.Pop(false);
        if (SourcesShape != IndexShape) { OutErrorMessage = TEXT("The first dimension sizes of Source must equal the shape of Index."); return {}; }

        FConcordShape SourceElementShape = SourceShape;
        SourceElementShape.RemoveAt(0, SourcesShape.Num(), false);
        TOptional<FConcordShape> OptionalBroadcastedShape = ConcordShape::Broadcast({ SourceElementShape, TargetElementShape });
        if (!OptionalBroadcastedShape) { OutErrorMessage = TEXT("Source element shape and target element shape could not be broadcasted together."); return {}; }
        ElementShape = OptionalBroadcastedShape.GetValue();

        FConcordShape SetShape = TargetShape;
        SetShape.SetNum(ActualDimensionIndex + 1);
        SetShape.Append(ElementShape);
        return { SetShape, GetSumType() };
    }

    int32 ActualDimensionIndex;
    FConcordShape ElementShape;
};
