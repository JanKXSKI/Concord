// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerRound.generated.h"

class FConcordRoundExpression : public FConcordComputingExpression
{
public:
    FConcordRoundExpression(TArray<FConcordSharedExpression>&& InSourceExpressions)
        : FConcordComputingExpression(MoveTemp(InSourceExpressions))
    {}

    FConcordValue ComputeValue(const FConcordExpressionContext<float>& Context) const override
    {
        return ConcordRound(SourceExpressions[0]->ComputeValue(Context).Float);
    }
#if WITH_EDITOR
    FString ToString() const override
    {
        return TEXT("\n        return ConcordRound($float0);\n");
    }
#endif
};

UCLASS()
class CONCORD_API UConcordTransformerRound : public UConcordTransformer
{
    GENERATED_BODY()
public:    
    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, EConcordValueType::Float, {}, "0,5"}
        };
    }

    FString GetCategory() const override { return TEXT("Conversion"); }

    FText GetDisplayName() const override { return INVTEXT("Round"); }
    FText GetTooltip() const override { return INVTEXT("Rounds the input floating point value to an integer value."); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& MultiIndex) const override
    {
        return MakeShared<const FConcordRoundExpression>(TArray<FConcordSharedExpression> { GetConnectedTransformers()[0]->GetExpression(MultiIndex) });
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        return { GetConnectedTransformers()[0]->GetShape(), EConcordValueType::Int };
    }
};
