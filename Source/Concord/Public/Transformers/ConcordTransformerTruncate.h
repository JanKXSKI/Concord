// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerTruncate.generated.h"

class FConcordTruncateExpression : public FConcordComputingExpression
{
public:
    FConcordTruncateExpression(FConcordSharedExpression&& InSourceExpression)
        : FConcordComputingExpression({ MoveTemp(InSourceExpression) })
    {}

    FConcordValue ComputeValue(const FConcordExpressionContext<float>& Context) const override
    {
        return ConcordTrunc(SourceExpressions[0]->ComputeValue(Context).Float);
    }
#if WITH_EDITOR
    FString ToString() const override
    {
        return TEXT("\n        return ConcordTrunc($float0);\n");
    }
#endif
};

UCLASS()
class CONCORD_API UConcordTransformerTruncate : public UConcordTransformer
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

    FText GetDisplayName() const override { return INVTEXT("Truncate"); }
    FText GetTooltip() const override { return INVTEXT("Truncates the input floating point value to an integer value."); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& MultiIndex) const override
    {
        return MakeShared<const FConcordTruncateExpression>(GetConnectedTransformers()[0]->GetExpression(MultiIndex));
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        return { GetConnectedTransformers()[0]->GetShape(), EConcordValueType::Int };
    }
};
