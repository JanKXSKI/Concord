// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerValue.generated.h"

UCLASS(Abstract)
class CONCORD_API UConcordTransformerValue : public UConcordTransformer
{
    GENERATED_BODY()
public:
    UPROPERTY()
    bool bIsPinDefaultValue;

    FString GetCategory() const override { return TEXT("Source"); }

    FText GetNodeDisplayName() const override { return INVTEXT("Value"); }
    FText GetTooltip() const override { return INVTEXT("Outputs a single value."); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& MultiIndex) const override
    {
        return SharedExpression.ToSharedRef();
    }
protected:
    TSharedPtr<const FConcordExpression> SharedExpression;
};

UCLASS()
class CONCORD_API UConcordTransformerValueInt : public UConcordTransformerValue
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Value")
    int32 Value;

    FText GetDisplayName() const override { return INVTEXT("Value (Int)"); }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        SharedExpression = MakeShared<const FConcordValueExpression<int32>>(Value);
        return { { 1 }, EConcordValueType::Int };
    }
};

UCLASS()
class CONCORD_API UConcordTransformerValueFloat : public UConcordTransformerValue
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Value")
    float Value;

    FText GetDisplayName() const override { return INVTEXT("Value (Float)"); }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        SharedExpression = MakeShared<const FConcordValueExpression<float>>(Value);
        return { { 1 }, EConcordValueType::Float };
    }
};
