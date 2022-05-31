// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerSwivel.generated.h"

UCLASS()
class CONCORD_API UConcordTransformerSwivel : public UConcordTransformer
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Swivel")
    int32 DimensionIndex;

    UConcordTransformerSwivel()
        : DimensionIndex(1)
        , ActualDimensionIndex(1)
    {}

    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, {}, {}, "0"}
        };
    }

    FString GetCategory() const override { return TEXT("Shape"); }

    FText GetDisplayName() const override { return INVTEXT("Swivel"); }
    FText GetTooltip() const override { return INVTEXT("Rotates the input shape until the specified dimension is at the front."); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& InMultiIndex) const override
    {
        return GetConnectedTransformers()[0]->GetExpression(Swivel(InMultiIndex, InMultiIndex.Num() - ActualDimensionIndex));
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        const FConcordShape& ConnectedShape = GetConnectedTransformers()[0]->GetShape();
        if (!GetActualDimensionIndex(DimensionIndex, ConnectedShape, ActualDimensionIndex, OutErrorMessage)) return {};
        return { Swivel(ConnectedShape, ActualDimensionIndex), GetConnectedTransformers()[0]->GetType() };
    }

    static TArray<int32> Swivel(const TArray<int32>& Array, int32 InDimensionIndex)
    {
        TArray<int32> Swivelled; Swivelled.Reserve(Array.Num());
        Swivelled.Append(Array.GetData() + InDimensionIndex, Array.Num() - InDimensionIndex);
        Swivelled.Append(Array.GetData(), InDimensionIndex);
        return MoveTemp(Swivelled);
    }

    int32 ActualDimensionIndex;
};
