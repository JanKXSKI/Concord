// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerFlankDistance.generated.h"

template<typename FValue>
class FConcordFlankDistanceExpression : public FConcordComputingExpression
{
public:
    FConcordFlankDistanceExpression(TArray<FConcordSharedExpression>&& InSourceExpressions)
        : FConcordComputingExpression(MoveTemp(InSourceExpressions))
    {}
    FConcordValue ComputeValue(const FConcordExpressionContext<float>& Context) const override;
#if WITH_EDITOR
    FString ToString() const override;
#endif
};

UCLASS()
class CONCORD_API UConcordTransformerFlankDistance : public UConcordTransformer
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Flank Distance")
    int32 DimensionIndex;

    UPROPERTY(EditAnywhere, Category = "Flank Distance")
    int32 MaxDistance;

    UPROPERTY(EditAnywhere, Category = "Flank Distance")
    bool bCyclic;

    UPROPERTY(EditAnywhere, Category = "Flank Distance")
    bool bBackwards;

    UConcordTransformerFlankDistance()
        : DimensionIndex(-1)
        , MaxDistance(4)
        , bCyclic(false)
        , bBackwards(false)
        , ActualDimensionIndex(0)
    {}

    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, {}, {}, "0"}
        };
    }

    FString GetCategory() const override { return TEXT("Sequence"); }

    FText GetDisplayName() const override { return INVTEXT("Flank Distance"); }
    FText GetTooltip() const override { return INVTEXT("Returns the number of equal values until a flank is reached."); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& InMultiIndex) const override
    {
        const int32 Num = GetConnectedTransformers()[0]->GetShape()[ActualDimensionIndex];
        FConcordMultiIndex MultiIndex = InMultiIndex;
        TArray<FConcordSharedExpression> SourceExpressions;
        const int32 Direction = bBackwards ? -1 : 1;
        for (int32 IndexOffset = 0; IndexOffset < FMath::Min(MaxDistance, Num); ++IndexOffset)
        {
            MultiIndex[ActualDimensionIndex] = InMultiIndex[ActualDimensionIndex] + Direction * IndexOffset;
            if (MultiIndex[ActualDimensionIndex] >= Num || MultiIndex[ActualDimensionIndex] < 0)
            {
                if (!bCyclic) break;
                MultiIndex[ActualDimensionIndex] += -Direction * Num;
            }
            SourceExpressions.Add(GetConnectedTransformers()[0]->GetExpression(MultiIndex));
        }
        if (SourceExpressions.Num() == 1) return MakeShared<const FConcordValueExpression<int32>>(bCyclic ? 0 : 1); // distance of last value to flank is 1 (only non-cyclic)
        switch (GetType())
        {
        case EConcordValueType::Int: return MakeShared<const FConcordFlankDistanceExpression<int32>>(MoveTemp(SourceExpressions));
        case EConcordValueType::Float: return MakeShared<const FConcordFlankDistanceExpression<float>>(MoveTemp(SourceExpressions));
        default: checkNoEntry(); return MakeShared<const FConcordValueExpression<int32>>(0);
        }
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        if (!GetActualDimensionIndex(DimensionIndex, GetConnectedTransformers()[0]->GetShape(), ActualDimensionIndex, OutErrorMessage)) return {};
        return { GetConnectedTransformers()[0]->GetShape(), EConcordValueType::Int };
    }

    int32 ActualDimensionIndex;
};
