// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerPR.generated.h"

class FConcordPRExpression : public FConcordComputingExpression
{
public:
    using FConcordComputingExpression::FConcordComputingExpression;
#if WITH_EDITOR
    FString ToString() const override { return TEXT("PR ToString() not implemented."); }
#endif
protected:
    int32 NumNotes() const;
    int32 GetNoteBoundaryIndex(const FConcordExpressionContext<float>& Context) const;
    int32 GetNote(int32 Index, const FConcordExpressionContext<float>& Context) const;

    template<typename FF>
    bool StepBackwards(FF F, int& Index) const
    {
        for (--Index; Index >= 0; --Index) if (F(Index)) return false;
        return true;
    }

    template<typename FF>
    bool StepForwards(FF F, int& Index) const
    {
        for (++Index; Index < NumNotes(); ++Index) if (F(Index)) return false;
        return true;
    }
};

class FConcordGPR2bExpression : public FConcordPRExpression
{
public:
    using FConcordPRExpression::FConcordPRExpression;
    FConcordValue ComputeValue(const FConcordExpressionContext<float>& Context) const override;
};

UCLASS()
class CONCORD_API UConcordTransformerPR : public UConcordTransformer
{
    GENERATED_BODY()
public:    
    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, EConcordValueType::Int, INVTEXT("Notes"), "0" },
            { {}, EConcordValueType::Int, INVTEXT("Boundary Note Index"), "0" }
        };
    }

    FString GetCategory() const override { return TEXT("GTTM"); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& InMultiIndex) const override
    {
        TArray<FConcordSharedExpression> SourceExpressions;
        AddBroadcastedConnectedExpressions(SourceExpressions, 1, 1, InMultiIndex);
        FConcordMultiIndex NotesIndex { InMultiIndex };
        ConcordShape::Unbroadcast(NotesIndex, NotesShape);
        for (NotesIndex.Add(0); NotesIndex.Last() < GetConnectedTransformers()[0]->GetShape().Last(); ++NotesIndex.Last())
            SourceExpressions.Add(GetConnectedTransformers()[0]->GetExpression(NotesIndex));
        return MakeExpression(MoveTemp(SourceExpressions));
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        NotesShape = GetConnectedTransformers()[0]->GetShape();
        NotesShape.Pop(false);
        TOptional<FConcordShape> BroadcastedShape = ConcordShape::Broadcast({ NotesShape, GetConnectedTransformers()[1]->GetShape() });
        if (!BroadcastedShape) { OutErrorMessage = TEXT("Input shapes could not be broadcasted together."); return {}; }
        return { BroadcastedShape.GetValue(), EConcordValueType::Int };
    }

    FConcordShape NotesShape;

    virtual FConcordSharedExpression MakeExpression(TArray<FConcordSharedExpression>&& SourceExpressions) const
    {
        checkNoEntry();
        return MakeShared<const FConcordValueExpression<int32>>(0);
    }
};

UCLASS()
class CONCORD_API UConcordTransformerGPR2b : public UConcordTransformerPR
{
    GENERATED_BODY()
public:
    FText GetDisplayName() const override { return INVTEXT("GPR2b"); }
    FText GetTooltip() const override { return INVTEXT("Tries to apply grouping preference rule 2b before the boundary note."); }
private:
    FConcordSharedExpression MakeExpression(TArray<FConcordSharedExpression>&& SourceExpressions) const override
    {
        return MakeShared<const FConcordGPR2bExpression>(MoveTemp(SourceExpressions));
    }
};
