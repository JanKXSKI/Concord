// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordCrate.h"
#include "ConcordParameter.generated.h"

UCLASS(Abstract)
class CONCORD_API UConcordParameter : public UConcordTransformer
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Parameter")
    TArray<int32> DefaultShape;

    UPROPERTY()
    bool bLocal;

    UPROPERTY(EditAnywhere, Category = "Parameter Default Crate")
    bool bGetDefaultValuesFromCrate;

    UPROPERTY(EditAnywhere, Category = "Parameter Default Crate", meta = (EditCondition = "bGetDefaultValuesFromCrate"))
    UConcordCrate* DefaultValuesCrate;

    UPROPERTY(EditAnywhere, Category = "Parameter Default Crate", meta = (EditCondition = "bGetDefaultValuesFromCrate"))
    FName DefaultValuesCrateBlockName;

    FText GetTooltip() const override
    {
        return INVTEXT("Parameters are used to feed values into a Concord model.");
    }

    TArray<FInputInfo> GetInputInfo() const override
    {
        return { { {}, {}, INVTEXT("Shape"), {} } };
    }

    FConcordValue GetDefaultValue(int32 FlatIndex) const
    {
        if (FlatIndex >= GetDefaultValuesNum())
        {
            if (GetDefaultValuesNum() == 1) FlatIndex = 0;
            else return 0;
        }
        if (bGetDefaultValuesFromCrate) switch (GetParameterType())
        {
        case EConcordValueType::Int: return DefaultValuesCrate->CrateData.IntBlocks[DefaultValuesCrateBlockName].Values[FlatIndex];
        case EConcordValueType::Float: return DefaultValuesCrate->CrateData.FloatBlocks[DefaultValuesCrateBlockName].Values[FlatIndex];
        default: checkNoEntry(); return 0;
        }
        else return GetNonCrateDefaultValue(FlatIndex);
    }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& MultiIndex) const override
    {
        return ExpressionDelegate.Execute(MultiIndex);
    }
private:
    friend class FConcordCompiler;
    DECLARE_DELEGATE_RetVal_OneParam(FConcordSharedExpression, FExpressionDelegate, const FConcordMultiIndex&);
    FExpressionDelegate ExpressionDelegate;

    TArray<int32> ComputeShape(TOptional<FString>& OutErrorMessage) const
    {
        if (!GetConnectedTransformers()[0]->IsPinDefaultValue()) return GetConnectedTransformers()[0]->GetShape();

        if (bGetDefaultValuesFromCrate)
        {
            if (!DefaultValuesCrate) OutErrorMessage = TEXT("No default values crate is set.");
            else switch (GetParameterType())
            {
            case EConcordValueType::Int:
                if (!DefaultValuesCrate->CrateData.IntBlocks.Contains(DefaultValuesCrateBlockName))
                    OutErrorMessage = TEXT("Default values crate block name does not name an integer block in the default values crate.");
                break;
            case EConcordValueType::Float: 
                if (!DefaultValuesCrate->CrateData.FloatBlocks.Contains(DefaultValuesCrateBlockName))
                    OutErrorMessage = TEXT("Default values crate block name does not name a float block in the default values crate.");
                break;
            default: checkNoEntry();
            }
            if (OutErrorMessage.IsSet()) return {};

        }
        TArray<int32> FittedShape = GetFittedDefaultShape();
        if (!ConcordShape::IsValidShape(FittedShape)) { OutErrorMessage = TEXT("Parameter shape invalid."); return {}; }
        return MoveTemp(FittedShape);
    }

    TArray<int32> GetFittedDefaultShape() const
    {
        TArray<int32> FittedShape = DefaultShape;
        if (FittedShape.IsEmpty()) FittedShape = { GetDefaultValuesNum() };
        return MoveTemp(FittedShape);
    }

    int32 GetDefaultValuesNum() const
    {
        if (bGetDefaultValuesFromCrate) switch (GetParameterType())
        {
        case EConcordValueType::Int: return DefaultValuesCrate->CrateData.IntBlocks[DefaultValuesCrateBlockName].Values.Num();
        case EConcordValueType::Float: return DefaultValuesCrate->CrateData.FloatBlocks[DefaultValuesCrateBlockName].Values.Num();
        default: checkNoEntry(); return 0;
        }
        else return GetNonCrateDefaultValuesNum();
    }

    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        return { ComputeShape(OutErrorMessage), GetParameterType() };
    }

    virtual EConcordValueType GetParameterType() const PURE_VIRTUAL(UConcordParameter::GetParameterType, return EConcordValueType::Error;)
    virtual FConcordValue GetNonCrateDefaultValue(int32 FlatIndex) const PURE_VIRTUAL(UConcordParameter::GetDefaultValue, return 0;)
    virtual int32 GetNonCrateDefaultValuesNum() const PURE_VIRTUAL(UConcordParameter::GetDefaultValuesNum, return 0;)
};

UCLASS()
class CONCORD_API UConcordParameterInt : public UConcordParameter
{
    GENERATED_BODY()
public:
    UConcordParameterInt()
    {
        DefaultValues.Add(0);
    }

    UPROPERTY(EditAnywhere, Category = "Parameter", meta=(EditCondition="!bGetDefaultValuesFromCrate"))
    TArray<int32> DefaultValues;

private:
    FConcordValue GetNonCrateDefaultValue(int32 FlatIndex) const override
    {
        return DefaultValues.Num() == 1 ? DefaultValues[0] : DefaultValues[FlatIndex];
    }

    int32 GetNonCrateDefaultValuesNum() const override
    {
        return DefaultValues.Num();
    }
    
    EConcordValueType GetParameterType() const override
    {
        return EConcordValueType::Int;
    }
};

UCLASS()
class CONCORD_API UConcordParameterFloat : public UConcordParameter
{
    GENERATED_BODY()
public:
    UConcordParameterFloat()
    {
        DefaultValues.Add(0.0f);
    }

    UPROPERTY(EditAnywhere, Category = "Parameter", meta=(EditCondition="!bGetDefaultValuesFromCrate"))
    TArray<float> DefaultValues;
private:
    FConcordValue GetNonCrateDefaultValue(int32 FlatIndex) const override
    {
        return DefaultValues.Num() == 1 ? DefaultValues[0] : DefaultValues[FlatIndex];
    }

    int32 GetNonCrateDefaultValuesNum() const override
    {
        return DefaultValues.Num();
    }

    EConcordValueType GetParameterType() const override
    {
        return EConcordValueType::Float;
    }
};
