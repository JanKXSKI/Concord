// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerPad.generated.h"

UCLASS(Abstract)
class CONCORD_API UConcordTransformerPad : public UConcordTransformer
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Pad")
    int32 DimensionIndex;
    
    UPROPERTY(EditAnywhere, Category = "Pad", meta=(ClampMin=0))
    int32 Count;

    UPROPERTY(EditAnywhere, Category = "Pad")
    bool bPadInFront;

    UConcordTransformerPad()
        : DimensionIndex(0)
        , Count(1)
    {}

    FString GetCategory() const override { return TEXT("Shape"); }

    FText GetNodeDisplayName() const override { return INVTEXT("Pad"); }
    FText GetTooltip() const override { return INVTEXT("Pads the input with a given value in a dimension."); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& MultiIndex) const override
    {
        if (bPadInFront && MultiIndex[ActualDimensionIndex] < Count)
            return SharedExpression.ToSharedRef();
        if (!bPadInFront && MultiIndex[ActualDimensionIndex] >= GetShape()[ActualDimensionIndex] - Count)
            return SharedExpression.ToSharedRef();
        FConcordMultiIndex PaddedMultiIndex = MultiIndex;
        if (bPadInFront) PaddedMultiIndex[ActualDimensionIndex] -= Count;
        PaddedMultiIndex.SetNum(GetConnectedTransformers()[0]->GetShape().Num());
        return GetConnectedTransformers()[0]->GetExpression(PaddedMultiIndex);
    }
protected:
    TSharedPtr<const FConcordExpression> SharedExpression;

    TArray<int32> ComputeShape(TOptional<FString>& OutErrorMessage)
    {
        TArray<int32> PaddedShape = GetConnectedTransformers()[0]->GetShape();
        PaddedShape.Add(1); // allow padding behind shape
        if (!GetActualDimensionIndex(DimensionIndex, PaddedShape, ActualDimensionIndex, OutErrorMessage)) return {};
        if (ActualDimensionIndex != PaddedShape.Num() - 1) PaddedShape.Pop();
        PaddedShape[ActualDimensionIndex] += Count;
        return PaddedShape;
    }

    int32 ActualDimensionIndex;
};

UCLASS()
class CONCORD_API UConcordTransformerPadInt : public UConcordTransformerPad
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Pad")
    int32 Value;

    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, EConcordValueType::Int, {}, "0"}
        };
    }

    FText GetDisplayName() const override { return INVTEXT("Pad (Int)"); }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        SharedExpression = MakeShared<const FConcordValueExpression<int32>>(Value);
        return { ComputeShape(OutErrorMessage), EConcordValueType::Int };
    }
};

UCLASS()
class CONCORD_API UConcordTransformerPadFloat : public UConcordTransformerPad
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Pad")
    float Value;

    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, EConcordValueType::Float, {}, "0"}
        };
    }

    FText GetDisplayName() const override { return INVTEXT("Pad (Float)"); }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        SharedExpression = MakeShared<const FConcordValueExpression<float>>(Value);
        return { ComputeShape(OutErrorMessage), EConcordValueType::Float };
    }
};
