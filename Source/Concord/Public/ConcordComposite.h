 // Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordVertex.h"
#include "ConcordTransformer.h"
#include "ConcordComposite.generated.h"

class UConcordModel;
class UConcordTransformer;
class UConcordOutput;

UCLASS()
class CONCORD_API UConcordComposite : public UConcordVertex
{
    GENERATED_BODY()
public:
    UPROPERTY()
    const UConcordModel* Model;

    FText GetDisplayName() const override;
    FText GetTooltip() const override;
    TArray<FInputInfo> GetInputInfo() const override;
    EConcordValueType GetOutputType(const FName& OutputName) const;
    class UConcordCompositeOutput* GetOutputTransformer(const FName& OutputName);
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override;
};

UCLASS()
class CONCORD_API UConcordCompositeOutput : public UConcordTransformer
{
    GENERATED_BODY()
public:
    FConcordSharedExpression GetExpression(const FConcordMultiIndex& MultiIndex) const override;
    UConcordComposite* GetComposite() const { return Cast<UConcordComposite>(GetOuter()); }
private:
    friend class FConcordCompiler;
    DECLARE_DELEGATE_RetVal_OneParam(FConcordSharedExpression, FExpressionDelegate, const FConcordMultiIndex&);
    FExpressionDelegate ExpressionDelegate;

    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override;
};
