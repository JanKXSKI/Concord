// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerTable.generated.h"

class CONCORD_API FConcordTransformerTableExpression : public FConcordComputingExpression
{
public:
    FConcordTransformerTableExpression(TArray<FConcordSharedExpression>&& InSourceExpressions, const FConcordShape& InTableShape)
        : FConcordComputingExpression(MoveTemp(InSourceExpressions))
        , TableShape(InTableShape)
    {}
    FConcordValue ComputeValue(const FConcordExpressionContext<float>& Context) const override;
#if WITH_EDITOR
    FString ToString() const override;
#endif
private:
    const FConcordShape TableShape;
};

UCLASS()
class CONCORD_API UConcordTransformerTable : public UConcordTransformer
{
    GENERATED_BODY()
public:
    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, {}, INVTEXT("Table"), "1"},
            { {}, EConcordValueType::Int, INVTEXT("Multi Index"), "0"}
        };
    }

    FText GetDisplayName() const override
    {
        return INVTEXT("Table");
    }

    FText GetTooltip() const override
    {
        return INVTEXT("Allows reading values from a table with a multi index.");
    }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& MultiIndex) const override
    {
        FConcordMultiIndex TablesMultiIndex = MultiIndex;
        FConcordMultiIndex MultiIndexMultiIndex = MultiIndex;
        ConcordShape::Unbroadcast(TablesMultiIndex, TablesShape);
        ConcordShape::Unbroadcast(MultiIndexMultiIndex, MultiIndexShape);
        TArray<FConcordSharedExpression> SourceExpressions;
        for (MultiIndexMultiIndex.Add(0); MultiIndexMultiIndex.Last() < TableShape.Num(); ++MultiIndexMultiIndex.Last())
            SourceExpressions.Add(GetConnectedTransformers()[1]->GetExpression(MultiIndexMultiIndex));
        ConcordShape::FShapeIterator It(TableShape);
        while (It.HasNext())
        {
            TablesMultiIndex.Append(It.Next());
            SourceExpressions.Add(GetConnectedTransformers()[0]->GetExpression(TablesMultiIndex));
            TablesMultiIndex.SetNum(TablesShape.Num(), false);
        }
        return MakeShared<const FConcordTransformerTableExpression>(MoveTemp(SourceExpressions), TableShape);
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        TablesShape = GetConnectedTransformers()[0]->GetShape();
        MultiIndexShape = GetConnectedTransformers()[1]->GetShape();
        const int32 TableDimensions = MultiIndexShape.Pop(false);
        if (TablesShape.Num() < TableDimensions) { OutErrorMessage = TEXT("Expected size of last dimension of MultiIndex to be smaller or equal to number of dimensions of Table."); return {}; }
        TableShape = TablesShape;
        TablesShape.SetNum(TablesShape.Num() - TableDimensions);
        TableShape.RemoveAt(0, TablesShape.Num());
        TOptional<FConcordShape> BroadcastedShape = ConcordShape::Broadcast({ MultiIndexShape, TablesShape });
        if (!BroadcastedShape) { OutErrorMessage = TEXT("Input shapes could not be broadcasted together."); return {}; }
        if (BroadcastedShape.GetValue().IsEmpty()) BroadcastedShape.GetValue().Add(1);
        return { BroadcastedShape.GetValue(), GetConnectedTransformers()[0]->GetType() };
    }

    FConcordShape MultiIndexShape;
    FConcordShape TablesShape;
    FConcordShape TableShape;
};
