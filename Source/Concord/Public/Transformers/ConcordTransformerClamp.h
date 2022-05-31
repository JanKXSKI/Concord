// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformerGet.h"
#include "ConcordTransformerClamp.generated.h"

template<typename FValue>
class FConcordClampExpression : public FConcordComputingExpression
{
public:
    FConcordClampExpression(TArray<FConcordSharedExpression>&& InSourceExpressions)
        : FConcordComputingExpression(MoveTemp(InSourceExpressions))
    {}
    FConcordValue ComputeValue(const FConcordExpressionContext<float>& Context) const override;
#if WITH_EDITOR
    FString ToString() const override;
#endif
};

UCLASS()
class CONCORD_API UConcordTransformerClamp : public UConcordTransformer
{
    GENERATED_BODY()
public:
    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, {}, INVTEXT("X"), "0"},
            { {}, {}, INVTEXT("Min"), "0"},
            { {}, {}, INVTEXT("Max"), "1"}
        };
    }

    FText GetDisplayName() const override { return INVTEXT("Clamp"); }
    FText GetTooltip() const override { return INVTEXT("Clamps input X to be between Min and Max (inclusive)."); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& MultiIndex) const override
    {
        switch (GetType())
        {
        case EConcordValueType::Int: return MakeShared<const FConcordClampExpression<int32>>(GetBroadcastedConnectedExpressions(MultiIndex, GetType()));
        case EConcordValueType::Float: return MakeShared<const FConcordClampExpression<float>>(GetBroadcastedConnectedExpressions(MultiIndex, GetType()));
        default: checkNoEntry(); return MakeShared<const FConcordValueExpression<int32>>(0);
        }
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        return { ComputeBroadcastedShape(OutErrorMessage), GetSumType() };
    }
};
