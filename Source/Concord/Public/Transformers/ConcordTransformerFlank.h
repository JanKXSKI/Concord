// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerFlank.generated.h"

template<typename FValue>
class FConcordFlankExpression : public FConcordComputingExpression
{
public:
    FConcordFlankExpression(TArray<FConcordSharedExpression>&& InSourceExpressions)
        : FConcordComputingExpression(MoveTemp(InSourceExpressions))
    {}
    FConcordValue ComputeValue(const FConcordExpressionContext<float>& Context) const override;
#if WITH_EDITOR
    FString ToString() const override;
#endif
};

UENUM()
enum class EConcordTransformerFlankMode : uint8
{
    StartWith0  UMETA(DisplayName = "Start with 0"),
    StartWith1  UMETA(DisplayName = "Start with 1"),
    Cyclic      UMETA(DisplayName = "Cyclic"),
};

UCLASS()
class CONCORD_API UConcordTransformerFlank : public UConcordTransformer
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Flank")
    int32 DimensionIndex;

    UPROPERTY(EditAnywhere, Category = "Flank")
    EConcordTransformerFlankMode Mode;

    UConcordTransformerFlank()
        : DimensionIndex(-1)
        , Mode(EConcordTransformerFlankMode::StartWith1)
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

    FText GetDisplayName() const override { return INVTEXT("Flank"); }
    FText GetTooltip() const override { return INVTEXT("Outputs 1 when a value is not equal to its predecessor, 0 otherwise."); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& InMultiIndex) const override
    {
        const int32 Num = GetConnectedTransformers()[0]->GetShape()[ActualDimensionIndex];
        FConcordMultiIndex PreviousMultiIndex = InMultiIndex;
        PreviousMultiIndex[ActualDimensionIndex] -= 1;
        if (PreviousMultiIndex[ActualDimensionIndex] == -1)
        {
            if (Mode == EConcordTransformerFlankMode::Cyclic) PreviousMultiIndex[ActualDimensionIndex] += Num;
            else return SharedValueExpression.ToSharedRef();
        }
        TArray<FConcordSharedExpression> SourceExpressions({ GetConnectedTransformers()[0]->GetExpression(PreviousMultiIndex),
                                                             GetConnectedTransformers()[0]->GetExpression(InMultiIndex) });
        switch (GetType())
        {
        case EConcordValueType::Int: return MakeShared<const FConcordFlankExpression<int32>>(MoveTemp(SourceExpressions));
        case EConcordValueType::Float: return MakeShared<const FConcordFlankExpression<float>>(MoveTemp(SourceExpressions));
        default: checkNoEntry(); return MakeShared<const FConcordValueExpression<int32>>(0);
        }
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        if (!GetActualDimensionIndex(DimensionIndex, GetConnectedTransformers()[0]->GetShape(), ActualDimensionIndex, OutErrorMessage)) return {};
        SharedValueExpression = MakeShared<const FConcordValueExpression<int32>>(Mode == EConcordTransformerFlankMode::StartWith0 ? 0 : 1);
        return { GetConnectedTransformers()[0]->GetShape(), EConcordValueType::Int };
    }

    int32 ActualDimensionIndex;
    TSharedPtr<const FConcordValueExpression<int32>> SharedValueExpression;
};
