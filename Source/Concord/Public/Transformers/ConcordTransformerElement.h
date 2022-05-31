// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerElement.generated.h"

UCLASS()
class CONCORD_API UConcordTransformerElement : public UConcordTransformer
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Element")
    TArray<int32> MultiIndex;

    UConcordTransformerElement()
    {
        MultiIndex.Add(0);
    }

    FString GetCategory() const override { return TEXT("Index"); }

    FText GetDisplayName() const override { return INVTEXT("Element"); }
    FText GetTooltip() const override { return INVTEXT("Statically gets an element from the input."); }

    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, {}, {}, "0"}
        };
    }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& InMultiIndex) const override
    {
        return GetConnectedTransformers()[0]->GetExpression(ActualMultiIndex);
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        const FConcordShape& ConnectedShape = GetConnectedTransformers()[0]->GetShape();
        while (MultiIndex.Num() < ConnectedShape.Num()) MultiIndex.Add(0);
        if (MultiIndex.Num() > ConnectedShape.Num()) { OutErrorMessage = TEXT("MultiIndex has more dimensions than connected shape."); return {}; }
        ActualMultiIndex = MultiIndex;
        for (int32 DimIndex = 0; DimIndex < MultiIndex.Num(); ++DimIndex)
            if (!GetActualIndex(MultiIndex[DimIndex], ConnectedShape[DimIndex], ActualMultiIndex[DimIndex], OutErrorMessage)) return {};
        return { { 1 }, GetConnectedTransformers()[0]->GetType() };
    }

    TArray<int32> ActualMultiIndex;
};
