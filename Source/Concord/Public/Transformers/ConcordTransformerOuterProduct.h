// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformerBinaryOperators.h"
#include "ConcordTransformerOuterProduct.generated.h"

UCLASS(HideCategories = (ConcordTransformerBinaryOperator))
class CONCORD_API UConcordTransformerOuterProduct : public UConcordTransformerMultiply
{
    GENERATED_BODY()
public:
    UConcordTransformerOuterProduct()
    {
        bOuter = true;
    }
    FText GetDisplayName() const override { return INVTEXT("Outer Product"); }
    FText GetNodeDisplayName() const override { return INVTEXT("Outer Product"); }
    FText GetTooltip() const override { return INVTEXT("Computes the outer product in the last dimension of the broadcasted inputs."); }
};
