// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerScale.generated.h"

class CONCORD_API FConcordToDegreeExpression : public FConcordComputingExpression
{
public:
    FConcordToDegreeExpression(TArray<FConcordSharedExpression>&& InSourceExpressions);
    FConcordValue ComputeValue(const FConcordExpressionContext<float>& Context) const override;
#if WITH_EDITOR
    FString ToString() const override;
#endif
};

class CONCORD_API FConcordToNoteExpression : public FConcordComputingExpression
{
public:
    FConcordToNoteExpression(TArray<FConcordSharedExpression>&& InSourceExpressions);
    FConcordValue ComputeValue(const FConcordExpressionContext<float>& Context) const override;
#if WITH_EDITOR
    FString ToString() const override;
#endif
};

UCLASS(Abstract)
class CONCORD_API UConcordTransformerScale : public UConcordTransformer
{
    GENERATED_BODY()
public:
    FString GetCategory() const override;
protected:
    TArray<FConcordSharedExpression> GetSourceExpressions(const FConcordMultiIndex& InMultiIndex) const;
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override;
};

UCLASS()
class CONCORD_API UConcordTransformerToDegree : public UConcordTransformerScale
{
    GENERATED_BODY()
public:
    FText GetDisplayName() const override;
    FText GetTooltip() const override;
    TArray<FInputInfo> GetInputInfo() const override;
    FConcordSharedExpression GetExpression(const FConcordMultiIndex& InMultiIndex) const override;
};

UCLASS()
class CONCORD_API UConcordTransformerToNote : public UConcordTransformerScale
{
    GENERATED_BODY()
public:
    FText GetDisplayName() const override;
    FText GetTooltip() const override;
    TArray<FInputInfo> GetInputInfo() const override;
    FConcordSharedExpression GetExpression(const FConcordMultiIndex& InMultiIndex) const override;
};
