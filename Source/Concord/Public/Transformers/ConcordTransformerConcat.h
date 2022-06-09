// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerConcat.generated.h"

UCLASS()
class CONCORD_API UConcordTransformerConcat : public UConcordAdaptiveTransformer
{
    GENERATED_BODY()
public:
    FString GetCategory() const override;
    FText GetDisplayName() const override;
    FText GetTooltip() const override;

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& MultiIndex) const override;
private:
    enum EMode { Flat, FlatArguments, Stack } Mode;
    TArray<int32> ConnectedShapeFlatNums;

    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override;
    TArray<int32> ComputeShape(TOptional<FString>& OutErrorMessage);
    EConcordValueType ComputeType(TOptional<FString>& OutErrorMessage);
};
