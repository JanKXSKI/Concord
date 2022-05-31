// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordVertex.h"
#include "ConcordTransformer.h"
#include "ConcordOutput.generated.h"

UCLASS()
class CONCORD_API UConcordOutput : public UConcordVertex
{
    GENERATED_BODY()
public:
    FText GetTooltip() const override
    {
        return INVTEXT("An output that can be queried from Blueprint.");
    }

    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, {}, INVTEXT("Output"), "0" }
        };
    }

    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        return { GetConnectedTransformers()[0]->GetShape(), GetConnectedTransformers()[0]->GetType() };
    }
};
