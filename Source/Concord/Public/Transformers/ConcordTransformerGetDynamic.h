// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerGetDynamic.generated.h"

class FConcordGetExpression : public FConcordComputingExpression
{
public:
    FConcordGetExpression(TArray<FConcordSharedExpression>&& InSourceExpressions)
        : FConcordComputingExpression(MoveTemp(InSourceExpressions))
    {}
    FConcordValue ComputeValue(const FConcordExpressionContext<float>& Context) const override;
#if WITH_EDITOR
    FString ToString() const override;
#endif
};

UCLASS()
class CONCORD_API UConcordTransformerGetDynamic : public UConcordTransformer
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Get (Dynamic)")
    int32 DimensionIndex;

    UConcordTransformerGetDynamic()
        : DimensionIndex(0)
        , ActualDimensionIndex(0)
        , IndexDimensionCount(0)
        , ValuesDimensionCount(0)
    {}

    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, {}, INVTEXT("Values"), "0"},
            { {}, EConcordValueType::Int, INVTEXT("Index"), "0"}
        };
    }

    FString GetCategory() const override { return TEXT("Index"); }

    FText GetDisplayName() const override { return INVTEXT("Get (Dynamic)"); }
    FText GetTooltip() const override { return INVTEXT("Dynamically gets part of the Values depending on the Index and the DimensionIndex."); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& InMultiIndex) const override
    {
        TArray<FConcordSharedExpression> SourceExpressions;
        const int32 Num = GetConnectedTransformers()[0]->GetShape()[ActualDimensionIndex];
        SourceExpressions.Reserve(2 + Num);

        const UConcordTransformer* IndexTransformer = GetConnectedTransformers()[1];
        if (IndexDimensionCount == 0)
        {
            SourceExpressions.Add(IndexTransformer->GetExpression({ 0 }));
        }
        else
        {
            FConcordMultiIndex IndexMultiIndex = InMultiIndex;
            IndexMultiIndex.SetNum(IndexDimensionCount);
            SourceExpressions.Add(IndexTransformer->GetExpression(IndexMultiIndex));
        }

        SourceExpressions.Add(SharedDefaultValueExpression.ToSharedRef());

        FConcordMultiIndex ValuesMultiIndex = InMultiIndex;
        ValuesMultiIndex.RemoveAt(0, IndexDimensionCount, false);
        ValuesMultiIndex.SetNum(ValuesDimensionCount, false);
        ValuesMultiIndex.Insert(0, ActualDimensionIndex);
        for (; ValuesMultiIndex[ActualDimensionIndex] < Num; ++ValuesMultiIndex[ActualDimensionIndex])
            SourceExpressions.Add(GetConnectedTransformers()[0]->GetExpression(ValuesMultiIndex));
        return MakeShared<const FConcordGetExpression>(MoveTemp(SourceExpressions));
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        FConcordShape ValuesShape = GetConnectedTransformers()[0]->GetShape();
        if (!GetActualDimensionIndex(DimensionIndex, ValuesShape, ActualDimensionIndex, OutErrorMessage)) return {};
        ValuesShape.RemoveAt(ActualDimensionIndex, 1, false);
        ValuesDimensionCount = ValuesShape.Num();
        TArray<int32> IndexShape = GetConnectedTransformers()[1]->GetShape();
        if (IndexShape.Num() == 1 && IndexShape[0] == 1) IndexShape.Reset();
        IndexDimensionCount = IndexShape.Num();
        SharedDefaultValueExpression = MakeShared<const FConcordValueExpression<int32>>(0);
        IndexShape.Append(ValuesShape);
        if (IndexShape.IsEmpty()) IndexShape = { 1 };
        return { IndexShape, GetConnectedTransformers()[0]->GetType() };
    }

    int32 ActualDimensionIndex;
    int32 IndexDimensionCount;
    int32 ValuesDimensionCount;
    TSharedPtr<const FConcordValueExpression<int32>> SharedDefaultValueExpression;
};
