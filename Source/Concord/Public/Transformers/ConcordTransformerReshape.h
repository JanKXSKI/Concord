// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerReshape.generated.h"

UCLASS()
class CONCORD_API UConcordTransformerReshape : public UConcordTransformer
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Reshape")
    TArray<int32> TargetShape;

    UConcordTransformerReshape()
    {
        TargetShape.Add(1);
    }

    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, {}, {}, "0"}
        };
    }

    FString GetCategory() const override { return TEXT("Shape"); }

    FText GetDisplayName() const override { return INVTEXT("Reshape"); }
    FText GetTooltip() const override { return INVTEXT("Reshapes the input to the TargetShape."); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& InMultiIndex) const override
    {
        const int32 FlatIndex = ConcordShape::FlattenIndex(InMultiIndex, GetShape());
        return GetConnectedTransformers()[0]->GetExpression(ConcordShape::UnflattenIndex(FlatIndex, GetConnectedTransformers()[0]->GetShape()));
    }
private:
    TArray<int32> ComputeShape(TOptional<FString>& OutErrorMessage)
    {
        if (TargetShape.IsEmpty()) { OutErrorMessage = TEXT("Invalid target shape."); return {}; }
        TArray<int32> ReshapedShape = TargetShape;
        const int32 TargetFlatNum = ConcordShape::GetFlatNum(GetConnectedTransformers()[0]->GetShape());
        int32 FlexibleIndex = -1;
        for (int32 DimIndex = 0; DimIndex < ReshapedShape.Num(); ++DimIndex)
            if (ReshapedShape[DimIndex] < 1)
            {
                if (FlexibleIndex != -1) { OutErrorMessage = TEXT("Invalid target shape."); return {}; }
                FlexibleIndex = DimIndex;
                ReshapedShape[FlexibleIndex] = 1;
            }
        if (FlexibleIndex != -1)
        {
            const float Quot = TargetFlatNum / ConcordShape::GetFlatNum(ReshapedShape);
            if (FMath::Frac(Quot) != 0.0f) { OutErrorMessage = TEXT("Cannot find integer size for flexible dimension."); return {}; }
            ReshapedShape[FlexibleIndex] = Quot;
        }
        if (ConcordShape::GetFlatNum(ReshapedShape) != TargetFlatNum) { OutErrorMessage = TEXT("Shapes do not have same number of elements."); return {}; }
        return ReshapedShape;
    }

    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        return { ComputeShape(OutErrorMessage), GetConnectedTransformers()[0]->GetType() };
    }
};
