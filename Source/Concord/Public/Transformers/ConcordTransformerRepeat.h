// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerRepeat.generated.h"

UCLASS()
class CONCORD_API UConcordTransformerRepeat : public UConcordTransformer
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Repeat")
    int32 DimensionIndex;
    
    UPROPERTY(EditAnywhere, Category = "Repeat")
    TArray<int32> RepeatShape;

    UConcordTransformerRepeat()
        : DimensionIndex(0)
    {
        RepeatShape.Add(1);
    }

    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, {}, {}, "0"}
        };
    }

    FString GetCategory() const override { return TEXT("Shape"); }

    FText GetDisplayName() const override { return INVTEXT("Repeat"); }
    FText GetTooltip() const override { return INVTEXT("Repeats the input in one dimension according to the RepeatShape."); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& MultiIndex) const override
    {
        if (bIsRepeatingScalar) return GetConnectedTransformers()[0]->GetExpression({ 0 });
        FConcordMultiIndex RepeatedMultiIndex = MultiIndex;
        RepeatedMultiIndex.RemoveAt(ActualDimensionIndex, RepeatShape.Num());
        return GetConnectedTransformers()[0]->GetExpression(RepeatedMultiIndex);
    }
private:
    TArray<int32> ComputeShape(TOptional<FString>& OutErrorMessage)
    {
        const FConcordShape& ConnectedShape = GetConnectedTransformers()[0]->GetShape();
        FConcordShape PaddedConnectedShape = ConnectedShape; PaddedConnectedShape.Add(1);
        if (!GetActualDimensionIndex(DimensionIndex, PaddedConnectedShape, ActualDimensionIndex, OutErrorMessage)) return {};
        if (RepeatShape.IsEmpty()) { OutErrorMessage = TEXT("Empty repeat shape."); return {}; }
        for (int32 Dim : RepeatShape) if (Dim <= 0) { OutErrorMessage = TEXT("Invalid repeat shape."); return {}; }
        bIsRepeatingScalar = ConnectedShape.Num() == 1 && ConnectedShape[0] == 1;
        if (bIsRepeatingScalar)
        {
            return RepeatShape;
        }
        else
        {
            TArray<int32> RepeatedShape = ConnectedShape;
            RepeatedShape.Insert(RepeatShape, ActualDimensionIndex);
            return RepeatedShape;
        }
    }

    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        return { ComputeShape(OutErrorMessage), GetConnectedTransformers()[0]->GetType() };
    }

    int32 ActualDimensionIndex;
    bool bIsRepeatingScalar;
};
