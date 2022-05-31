// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordBox.generated.h"

// A box is a transformer without inputs.
UCLASS()
class CONCORD_API UConcordBox : public UConcordTransformer
{
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<int32> DefaultShape;

    UPROPERTY()
    int32 StateCount;

    FText GetTooltip() const override
    {
        return INVTEXT("A box containing random variables.");
    }

    TArray<FInputInfo> GetInputInfo() const override
    {
        return { { {}, {}, INVTEXT("Shape"), {} } };
    }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& MultiIndex) const override
    {
        return ExpressionDelegate.Execute(MultiIndex);
    }
private:
    friend class FConcordCompiler;
    DECLARE_DELEGATE_RetVal_OneParam(FConcordSharedExpression, FExpressionDelegate, const FConcordMultiIndex&);
    FExpressionDelegate ExpressionDelegate;

    TArray<int32> ComputeShape(TOptional<FString>& OutErrorMessage) const
    {
        if (!GetConnectedTransformers()[0]->IsPinDefaultValue()) return GetConnectedTransformers()[0]->GetShape();
        if (!ConcordShape::IsValidShape(DefaultShape)) { OutErrorMessage = TEXT("Invalid shape."); return {}; }
        return DefaultShape;
    }

    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        return { ComputeShape(OutErrorMessage), EConcordValueType::Int };
    }
};

// State set is a transformer without inputs, useful for adaptive shapes on trainable float parameters
UCLASS()
class CONCORD_API UConcordBoxStateSet : public UConcordTransformer
{
    GENERATED_BODY()
public:
    FConcordSharedExpression GetExpression(const FConcordMultiIndex& MultiIndex) const override
    {
        return SharedExpressions[MultiIndex[0]];
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        const int32 StateCount = Cast<UConcordBox>(GetOuter())->StateCount;
        SharedExpressions.Empty(StateCount);
        for (int32 State = 0; State < StateCount; ++State)
            SharedExpressions.Add(MakeShared<const FConcordValueExpression<int32>>(State));
        return { { StateCount }, EConcordValueType::Int };
    }

    TArray<FConcordSharedExpression> SharedExpressions;
};
