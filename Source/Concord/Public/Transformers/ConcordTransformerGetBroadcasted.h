// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformerGetDynamic.h"
#include "ConcordTransformerGetBroadcasted.generated.h"

UCLASS()
class CONCORD_API UConcordTransformerGetBroadcasted : public UConcordTransformer
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Get (Broadcasted)")
    int32 DimensionIndex;
    
    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, {}, INVTEXT("Values"), "0"},
            { {}, EConcordValueType::Int, INVTEXT("Index"), "0"}
        };
    }

    FString GetCategory() const override { return TEXT("Index"); }

    FText GetDisplayName() const override { return INVTEXT("Get (Broadcasted)"); }
    FText GetTooltip() const override { return INVTEXT("Dynamically gets part of the broadcasted Values based on the Index and the DimensionIndex."); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& InMultiIndex) const override
    {
        TArray<FConcordSharedExpression> SourceExpressions;
        SourceExpressions.Reserve(2 + GetConnectedTransformers()[0]->GetShape()[ActualDimensionIndex]);

        const UConcordTransformer* IndexTransformer = GetConnectedTransformers()[1];
        FConcordMultiIndex IndexMultiIndex = InMultiIndex;
        ConcordShape::Unbroadcast(IndexMultiIndex, IndexTransformer->GetShape());
        SourceExpressions.Add(IndexTransformer->GetExpression(IndexMultiIndex));

        SourceExpressions.Add(SharedDefaultValueExpression.ToSharedRef());

        const UConcordTransformer* ValuesTransformer = GetConnectedTransformers()[0];
        FConcordShape ValuesShape = ValuesTransformer->GetShape();
        const int32 Num = ValuesShape[ActualDimensionIndex];
        ValuesShape.RemoveAt(ActualDimensionIndex, 1, false);
        FConcordMultiIndex ValuesMultiIndex = InMultiIndex;
        ConcordShape::Unbroadcast(ValuesMultiIndex, ValuesShape);
        for (ValuesMultiIndex.Insert(0, ActualDimensionIndex); ValuesMultiIndex[ActualDimensionIndex] < Num; ++ValuesMultiIndex[ActualDimensionIndex])
            SourceExpressions.Add(ValuesTransformer->GetExpression(ValuesMultiIndex));
        return MakeShared<const FConcordGetExpression>(MoveTemp(SourceExpressions));
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        TArray<FConcordShape> ConnectedShapes = GetConnectedShapes();
        if (!GetActualDimensionIndex(DimensionIndex, ConnectedShapes[0], ActualDimensionIndex, OutErrorMessage)) return {};
        ConnectedShapes[0].RemoveAt(ActualDimensionIndex, 1, false);
        TOptional<FConcordShape> BroadcastedShape = ConcordShape::Broadcast(ConnectedShapes);
        if (!BroadcastedShape) { OutErrorMessage = TEXT("Input shapes could not be broadcasted together."); return {}; }
        SharedDefaultValueExpression = MakeShared<const FConcordValueExpression<int32>>(0);
        return { BroadcastedShape.GetValue(), GetConnectedTransformers()[0]->GetType() };
    }

    int32 ActualDimensionIndex;
    TSharedPtr<const FConcordValueExpression<int32>> SharedDefaultValueExpression;
};
