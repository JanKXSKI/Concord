// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerCast.generated.h"

class FConcordCastExpression : public FConcordComputingExpression
{
public:
    FConcordCastExpression(FConcordSharedExpression&& InSourceExpression)
        : FConcordComputingExpression({ MoveTemp(InSourceExpression) })
    {}

    FConcordValue ComputeValue(const FConcordExpressionContext<float>& Context) const override
    {
        return float(SourceExpressions[0]->ComputeValue(Context).Int);
    }
#if WITH_EDITOR
    FString ToString() const override
    {
        return TEXT("\n        return $int0;\n");
    }
#endif
};

UCLASS()
class CONCORD_API UConcordTransformerCast : public UConcordTransformer
{
    GENERATED_BODY()
public:
    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, EConcordValueType::Int, {}, "0" }
        };
    }
    
    FString GetCategory() const override { return TEXT("Conversion"); }

    FText GetDisplayName() const override { return INVTEXT("Cast"); }
    FText GetTooltip() const override { return INVTEXT("Casts an integer value to a floating point value."); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& MultiIndex) const override
    {
        return MakeShared<const FConcordCastExpression>(GetConnectedTransformers()[0]->GetExpression(MultiIndex));
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        return { GetConnectedTransformers()[0]->GetShape(), EConcordValueType::Float };
    }
};
