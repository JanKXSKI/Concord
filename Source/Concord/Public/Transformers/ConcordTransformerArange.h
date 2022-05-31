// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerArange.generated.h"

UCLASS()
class CONCORD_API UConcordTransformerArange : public UConcordTransformer
{
    GENERATED_BODY()
public:
    UConcordTransformerArange()
        : Start(0)
        , Step(1)
        , Last(9)
    {}

    UPROPERTY(EditAnywhere, Category = "Arange")
    int32 Start;

    UPROPERTY(EditAnywhere, Category = "Arange", meta=(ClampMin=1))
    int32 Step;

    UPROPERTY(EditAnywhere, Category = "Arange")
    int32 Last;

    FString GetCategory() const override { return TEXT("Source"); }

    FText GetDisplayName() const override { return INVTEXT("Arange"); }
    FText GetTooltip() const override { return INVTEXT("Outputs ascending numbers."); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& InMultiIndex) const override
    {
        return SharedExpressions[InMultiIndex[0]];
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        if (Start >= Last) { OutErrorMessage = TEXT("Start needs to be smaller than last."); return {}; }
        const int32 FlatNum = (Last - Start + Step) / Step;
        SharedExpressions.Empty(FlatNum);
        for (int32 Value = Start; Value <= Last; Value += Step)
            SharedExpressions.Add(MakeShared<const FConcordValueExpression<int32>>(Value));
        return { { FlatNum }, EConcordValueType::Int };
    }

    TArray<FConcordSharedExpression> SharedExpressions;
};
