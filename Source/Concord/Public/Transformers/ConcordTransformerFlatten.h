// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerFlatten.generated.h"

UCLASS()
class CONCORD_API UConcordTransformerFlatten : public UConcordTransformer
{
    GENERATED_BODY()
public:
    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, {}, {}, "0"}
        };
    }
    
    FString GetCategory() const override { return TEXT("Shape"); }

    FText GetDisplayName() const override { return INVTEXT("Flatten"); }
    FText GetTooltip() const override { return INVTEXT("Flattens the input's shape."); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& InMultiIndex) const override
    {
        return GetConnectedTransformers()[0]->GetExpression(ConcordShape::UnflattenIndex(InMultiIndex[0], GetConnectedTransformers()[0]->GetShape()));
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        return { { ConcordShape::GetFlatNum(GetConnectedTransformers()[0]->GetShape()) }, GetConnectedTransformers()[0]->GetType() };
    }
};
