// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerPositionInOctave.generated.h"

class CONCORD_API FConcordPositionInOctaveExpression : public FConcordComputingExpression
{
public:
    FConcordPositionInOctaveExpression(TArray<FConcordSharedExpression>&& InSourceExpressions)
        : FConcordComputingExpression(MoveTemp(InSourceExpressions))
    {}
    FConcordValue ComputeValue(const FConcordExpressionContext<float>& Context) const override;
#if WITH_EDITOR
    FString ToString() const override;
#endif
};

UCLASS()
class CONCORD_API UConcordTransformerPositionInOctave : public UConcordTransformer
{
    GENERATED_BODY()
public:
    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, EConcordValueType::Int, INVTEXT("Root"), "60"},
            { {}, EConcordValueType::Int, INVTEXT("Note"), "60"}
        };
    }

    FString GetCategory() const override { return TEXT("Music"); }

    FText GetDisplayName() const override { return INVTEXT("Position in Octave"); }
    FText GetTooltip() const override { return INVTEXT("Returns the position (0-11) of the input Note in its octave."); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& InMultiIndex) const override
    {
        return MakeShared<const FConcordPositionInOctaveExpression>(GetBroadcastedConnectedExpressions(InMultiIndex));
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        return { ComputeBroadcastedShape(OutErrorMessage), EConcordValueType::Int };
    }
};
