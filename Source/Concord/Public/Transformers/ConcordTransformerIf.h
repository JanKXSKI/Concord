// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformerGetDynamic.h"
#include "ConcordTransformerIf.generated.h"

UCLASS()
class CONCORD_API UConcordTransformerIf : public UConcordTransformer
{
    GENERATED_BODY()
public:
    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, EConcordValueType::Int, INVTEXT("Condition"), "1"},
            { {}, {}, INVTEXT("True"), "1"},
            { {}, {}, INVTEXT("False"), "0"}
        };
    }

    FText GetDisplayName() const override { return INVTEXT("If"); }
    FText GetTooltip() const override { return INVTEXT("Outputs the True input or the False input, depending on whether the Condition."); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& MultiIndex) const override
    {
        TArray<FConcordSharedExpression> SourceExpressions; SourceExpressions.Reserve(3);
        AddBroadcastedConnectedExpressions(SourceExpressions, 0, 1, MultiIndex);
        AddBroadcastedConnectedExpressions(SourceExpressions, 1, 2, MultiIndex, GetType());
        return MakeShared<const FConcordGetExpression>(MoveTemp(SourceExpressions));
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        return { ComputeBroadcastedShape(OutErrorMessage), GetSumType() };
    }
};
