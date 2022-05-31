// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerNum.generated.h"

UCLASS()
class CONCORD_API UConcordTransformerNum : public UConcordTransformer
{
    GENERATED_BODY()
public:
    UConcordTransformerNum() : DimensionIndex(0)
    {}

    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, {}, {}, "0"}
        };
    }

    UPROPERTY(EditAnywhere, Category = "Num")
    int32 DimensionIndex;

    FString GetCategory() const override { return TEXT("Shape"); }

    FText GetDisplayName() const override { return INVTEXT("Num"); }
    FText GetTooltip() const override { return INVTEXT("Returns the number of elements along a dimension of the input."); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& InMultiIndex) const override
    {
        return SharedExpression.ToSharedRef();
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        const FConcordShape& ConnectedShape = GetConnectedTransformers()[0]->GetShape();
        int32 ActualDimensionIndex;
        if (!GetActualDimensionIndex(DimensionIndex, ConnectedShape, ActualDimensionIndex, OutErrorMessage)) return {};
        SharedExpression = MakeShared<const FConcordValueExpression<int32>>(ConnectedShape[ActualDimensionIndex]);
        return { { 1 }, EConcordValueType::Int };
    }

    TSharedPtr<const FConcordExpression> SharedExpression;
};
