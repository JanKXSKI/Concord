// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordBox.h"
#include "ConcordEmission.generated.h"

UCLASS()
class CONCORD_API UConcordEmission : public UConcordVertex
{
    GENERATED_BODY()
public:
    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, {}, INVTEXT("Hidden"), "0" },
            { {}, {}, INVTEXT("Observed"), "0" }
        };
    }

    FText GetDisplayName() const override
    {
        return INVTEXT("Emission");
    }

    FText GetTooltip() const override
    {
        return INVTEXT("Emission nodes connect the all pins of random variable boxes to build HMMs.");
    }

    UConcordBox* GetConnectedBox(int32 Index) { return Cast<UConcordBox>(GetConnectedTransformers()[Index]); }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        UConcordBox* HiddenBox = GetConnectedBox(0);
        UConcordBox* ObservedBox = GetConnectedBox(1);
        if (!HiddenBox || !ObservedBox)
        {
            OutErrorMessage = TEXT("Emission input must be connected to a single All pin of a box.");
            return {};
        }
        if (HiddenBox->GetShape().Num() > ObservedBox->GetShape().Num())
        {
            OutErrorMessage = TEXT("Hidden box dimension count must be less than or equal to observed box dimension count.");
            return {};
        }
        for (int32 DimIndex = 0; DimIndex < HiddenBox->GetShape().Num(); ++DimIndex)
            if (HiddenBox->GetShape()[DimIndex] != ObservedBox->GetShape()[DimIndex])
            {
                OutErrorMessage = TEXT("Hidden box shape and observed box shape must match up to hidden box shape dimension count.");
                return {};
            }
        return { {}, EConcordValueType::Float };
    }
};
