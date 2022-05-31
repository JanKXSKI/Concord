// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "Algo/MaxElement.h"
#include "ConcordTransformerStack.generated.h"

UCLASS()
class CONCORD_API UConcordTransformerStack : public UConcordAdaptiveTransformer
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Stack")
    int32 DimensionIndex;

    UConcordTransformerStack()
        : DimensionIndex(0)
        , ActualDimensionIndex(0)
    {}

    FString GetCategory() const override { return TEXT("Shape"); }

    FText GetDisplayName() const override { return INVTEXT("Stack"); }
    FText GetTooltip() const override { return INVTEXT("Stacks the inputs in the dimension specified."); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& MultiIndex) const override
    {
        for (int32 Index = 0; Index < Cutoffs.Num(); ++Index)
        {
            if (MultiIndex[ActualDimensionIndex] < Cutoffs[Index])
            {
                FConcordMultiIndex StackedMultiIndex = MultiIndex;
                if (Index > 0) StackedMultiIndex[ActualDimensionIndex] -= Cutoffs[Index - 1];
                ConcordShape::Unbroadcast(StackedMultiIndex, GetConnectedTransformers()[Index]->GetShape());
                return GetConnectedTransformers()[Index]->GetExpression(StackedMultiIndex);
            }
        }
        checkNoEntry(); return MakeShared<const FConcordValueExpression<int32>>(0);
    }
private:
    TArray<int32> ComputeShape(TOptional<FString>& OutErrorMessage)
    {
        TArray<FConcordShape> ConnectedShapes = GetConnectedShapes();
        FConcordShape BiggestShape = *Algo::MaxElementBy(ConnectedShapes, [](const auto& A){ return A.Num(); });
        BiggestShape.Add(1); // allow stacking behind shape
        if (!GetActualDimensionIndex(DimensionIndex, BiggestShape, ActualDimensionIndex, OutErrorMessage)) return {};
        int32 StackedDimSize = 0;
        Cutoffs.Reset();
        for (auto& ConnectedShape : ConnectedShapes)
        {
            while (ConnectedShape.Num() < BiggestShape.Num()) ConnectedShape.Add(1);
            StackedDimSize += ConnectedShape[ActualDimensionIndex];
            Cutoffs.Add(StackedDimSize);
            ConnectedShape.RemoveAt(ActualDimensionIndex, 1, false);
        }
        TOptional<FConcordShape> BroadcastedShape = ConcordShape::Broadcast(ConnectedShapes);
        if (!BroadcastedShape) { OutErrorMessage = TEXT("Input shapes could not be broadcasted together."); return {}; }
        FConcordShape& StackedShape = BroadcastedShape.GetValue();
        StackedShape.Insert(StackedDimSize, ActualDimensionIndex);
        if (StackedShape.Last() == 1) StackedShape.Pop();
        return StackedShape;
    }

    EConcordValueType ComputeType(TOptional<FString>& OutErrorMessage)
    {
        EConcordValueType FirstType = GetConnectedTransformers()[0]->GetType();
        for (int32 Index = 1; Index < GetConnectedTransformers().Num(); ++Index)
            if (FirstType != GetConnectedTransformers()[Index]->GetType()) { OutErrorMessage = TEXT("Input types must match."); return EConcordValueType::Error; }
        return FirstType;
    }

    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        return { ComputeShape(OutErrorMessage), ComputeType(OutErrorMessage) };
    }

    int32 ActualDimensionIndex;
    TArray<int32> Cutoffs;
};
