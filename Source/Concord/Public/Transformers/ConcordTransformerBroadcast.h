// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerBroadcast.generated.h"

UCLASS()
class CONCORD_API UConcordTransformerBroadcast : public UConcordTransformer
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Broadcast")
    TArray<int32> TargetShape;

    UConcordTransformerBroadcast()
    {
        TargetShape.Add(1);
    }

    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, {}, {}, "0" }
        };
    }

    FString GetCategory() const override { return TEXT("Shape"); }

    FText GetDisplayName() const override { return INVTEXT("Broadcast"); }
    FText GetTooltip() const override { return INVTEXT("Broadcasts the input shape to the given TargetShape."); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& InMultiIndex) const override
    {
        FConcordMultiIndex UnbroadcastMultiIndex = InMultiIndex;
        ConcordShape::Unbroadcast(UnbroadcastMultiIndex, GetConnectedTransformers()[0]->GetShape());
        return GetConnectedTransformers()[0]->GetExpression(UnbroadcastMultiIndex);
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        if (TargetShape.IsEmpty()) { OutErrorMessage = TEXT("Invalid target shape."); return {}; }
        for (int32 Dim : TargetShape) if (Dim < 1) { OutErrorMessage = TEXT("Invalid target shape."); return {}; }
        const FConcordShape& ConnectedShape = GetConnectedTransformers()[0]->GetShape();
        if (TargetShape.Num() < ConnectedShape.Num()) { OutErrorMessage = TEXT("Target shape has fewer dimensions than input shape."); return {}; }
        for (int32 DimIndex = 0; DimIndex < ConnectedShape.Num(); ++DimIndex)
            if (TargetShape[DimIndex] != ConnectedShape[DimIndex] && ConnectedShape[DimIndex] > 1)
            {
                OutErrorMessage = FString::Printf(TEXT("Cannot broadcast to target shape, mismatch in dimension %i."), DimIndex);
                return {};
            }
        return { TargetShape, GetConnectedTransformers()[0]->GetType() };
    }
};
