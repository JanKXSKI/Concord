 // Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordVertex.h"
#include "ConcordTransformer.h"
#include "FactorGraph/ConcordFactorGraph.h"
#include "ConcordInstance.generated.h"

class UConcordModelBase;
class UConcordTransformer;
class UConcordOutput;
class UConcordCrate;

UCLASS()
class CONCORD_API UConcordInstance : public UConcordVertex
{
    GENERATED_BODY()
public:
    UPROPERTY()
    const UConcordModelBase* Model;

    UPROPERTY(EditAnywhere, Category = "Instance")
    TArray<UConcordCrate*> InstanceCrates;

    void UpdateCachedInputOutputInfo() const;
    FText GetDisplayName() const override;
    FText GetTooltip() const override;
    TArray<FInputInfo> GetInputInfo() const override;
    EConcordValueType GetOutputType(const FName& OutputName) const;
    class UConcordInstanceOutput* GetOutputTransformer(const FName& OutputName);
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override;

    mutable TArray<UConcordVertex::FInputInfo> CachedInputInfo;
    mutable TMap<FName, TTuple<int32, EConcordValueType>> CachedOutputInfo;
    friend class UConcordInstanceOutput;
};

UCLASS()
class CONCORD_API UConcordInstanceOutput : public UConcordTransformer
{
    GENERATED_BODY()
public:
    FConcordSharedExpression GetExpression(const FConcordMultiIndex& MultiIndex) const override;
    UConcordInstance* GetInstance() const { return Cast<UConcordInstance>(GetOuter()); }
private:
    friend class FConcordCompiler;
    DECLARE_DELEGATE_RetVal_OneParam(FConcordSharedExpression, FExpressionDelegate, const FConcordMultiIndex&);
    FExpressionDelegate ExpressionDelegate;

    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override;
};
