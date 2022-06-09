// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordFactor.generated.h"

UCLASS()
class CONCORD_API UConcordFactor : public UConcordTransformer
{
    GENERATED_BODY()
public:
    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, {}, INVTEXT("Score"), "1" }
        };
    }

    FText GetDisplayName() const override
    {
        return INVTEXT("Factor");
    }

    FText GetTooltip() const override
    {
        return INVTEXT("A factor that influences the model's probability distribution.");
    }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& MultiIndex) const override
    {
        return GetConnectedExpression(GetConnectedTransformers()[0], MultiIndex, EConcordValueType::Float);
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        return { GetConnectedTransformers()[0]->GetShape(), EConcordValueType::Float };
    }
};
