// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerChordRoot.generated.h"

class FConcordChordRootExpression : public FConcordComputingExpression
{
public:
    FConcordChordRootExpression(TArray<FConcordSharedExpression>&& InSourceExpressions)
        : FConcordComputingExpression(MoveTemp(InSourceExpressions))
    {}
    FConcordValue ComputeValue(const FConcordExpressionContext<float>& Context) const override;
#if WITH_EDITOR
    FString ToString() const override;
#endif
};

UCLASS()
class CONCORD_API UConcordTransformerChordRoot : public UConcordTransformer
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

    FString GetCategory() const override { return TEXT("Music"); }

    FText GetDisplayName() const override { return INVTEXT("Chord Root"); }
    FText GetTooltip() const override { return INVTEXT("Gets the root of the chords in the last dimension of the input, checks second inversions."); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& InMultiIndex) const override
    {
        const auto& ConnectedShape = GetConnectedTransformers()[0]->GetShape();
        TArray<FConcordSharedExpression> SourceExpressions; SourceExpressions.Reserve(ConnectedShape.Last());
        FConcordMultiIndex MultiIndex = InMultiIndex;
        if (ConnectedShape.Num() == 1) MultiIndex.Reset();
        for (MultiIndex.Add(0); MultiIndex.Last() < ConnectedShape.Last(); ++MultiIndex.Last())
            SourceExpressions.Add(GetConnectedTransformers()[0]->GetExpression(MultiIndex));
        return MakeShared<FConcordChordRootExpression>(MoveTemp(SourceExpressions));
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        auto ChordRootShape = GetConnectedTransformers()[0]->GetShape();
        ChordRootShape.Pop();
        if (ChordRootShape.IsEmpty()) ChordRootShape.Add(1);
        return { ChordRootShape, EConcordValueType::Int };
    }
};
