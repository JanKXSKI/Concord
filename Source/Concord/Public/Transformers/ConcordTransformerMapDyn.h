// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerMapDyn.generated.h"

class FConcordMapDynExpression : public FConcordComputingExpression
{
public:
    FConcordMapDynExpression(TArray<FConcordSharedExpression>&& InSourceExpressions)
        : FConcordComputingExpression(MoveTemp(InSourceExpressions))
        , NumPairs((SourceExpressions.Num() - 2) / 2)
    {}
    FConcordValue ComputeValue(const FConcordExpressionContext<float>& Context) const override;
#if WITH_EDITOR
    FString ToString() const override;
#endif
private:
    const int32 NumPairs;
};

UCLASS()
class CONCORD_API UConcordTransformerMapDyn : public UConcordTransformer
{
    GENERATED_BODY()
public:
    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, EConcordValueType::Int, INVTEXT("Keys"), "0"},
            { {}, {}, INVTEXT("Values"), "0"},
            { {}, EConcordValueType::Int, INVTEXT("Key"), "0"},
            { {}, {}, INVTEXT("Default"), "0"}
        };
    }

    FText GetDisplayName() const override { return INVTEXT("Map (Dynamic)"); }
    FText GetNodeDisplayName() const override { return INVTEXT("Map"); }
    FText GetTooltip() const override { return INVTEXT("Dynamically maps input integers to output values."); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& InMultiIndex) const override
    {
        TArray<FConcordSharedExpression> SourceExpressions;
        SourceExpressions.Reserve(2 + 2 * GetConnectedTransformers()[0]->GetShape().Last());

        for (int32 Index = 2; Index < 4; ++Index)
        {
            const UConcordTransformer* ConnectedTransformer = GetConnectedTransformers()[Index];
            FConcordMultiIndex ConnectedMultiIndex = InMultiIndex;
            ConcordShape::Unbroadcast(ConnectedMultiIndex, ConnectedTransformer->GetShape());
            SourceExpressions.Add(ConnectedTransformer->GetExpression(ConnectedMultiIndex));
        }

        for (int32 Index = 0; Index < 2; ++Index)
        {
            const UConcordTransformer* ConnectedTransformer = GetConnectedTransformers()[Index];
            FConcordShape ConnectedShape = ConnectedTransformer->GetShape();
            const int32 Num = ConnectedShape.Pop(false);
            FConcordMultiIndex ConnectedMultiIndex = InMultiIndex;
            ConcordShape::Unbroadcast(ConnectedMultiIndex, ConnectedShape);
            for (ConnectedMultiIndex.Add(0); ConnectedMultiIndex.Last() < Num; ++ConnectedMultiIndex.Last())
                SourceExpressions.Add(ConnectedTransformer->GetExpression(ConnectedMultiIndex));
        }
        return MakeShared<const FConcordMapDynExpression>(MoveTemp(SourceExpressions));
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        TArray<FConcordShape> ConnectedShapes = GetConnectedShapes();
        if (ConnectedShapes[0].Pop(false) != ConnectedShapes[1].Pop(false)) { OutErrorMessage = TEXT("The size of the last dimension of Keys and Values must match."); return {}; }
        TOptional<FConcordShape> BroadcastedShape = ConcordShape::Broadcast(ConnectedShapes);
        if (!BroadcastedShape) { OutErrorMessage = TEXT("Input shapes could not be broadcasted together."); return {}; }

        const EConcordValueType ValuesType = GetConnectedTransformers()[1]->GetType();
        if (GetConnectedTransformers()[3]->GetType() != ValuesType) { OutErrorMessage = TEXT("Types of Values and Default must match."); return {}; }

        return { BroadcastedShape.GetValue(), ValuesType };
    }
};
