// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerMap.generated.h"

template<typename FValue>
class FConcordMapExpression : public FConcordComputingExpression
{
public:
    FConcordMapExpression(TArray<FConcordSharedExpression>&& InSourceExpressions, const TSharedRef<const TMap<int32, FValue>>& InMap, FValue InDefaultValue)
        : FConcordComputingExpression(MoveTemp(InSourceExpressions))
        , Map(InMap)
        , DefaultValue(InDefaultValue)
    {}
    FConcordValue ComputeValue(const FConcordExpressionContext<float>& Context) const override;
#if WITH_EDITOR
    FString ToString() const override;
#endif
private:
    const TSharedRef<const TMap<int32, FValue>> Map;
    const FValue DefaultValue;
};

UCLASS(Abstract)
class CONCORD_API UConcordTransformerMap : public UConcordTransformer
{
    GENERATED_BODY()
public:
    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, EConcordValueType::Int, {}, "0"}
        };
    }

    FText GetNodeDisplayName() const override { return INVTEXT("Map"); }
};

UCLASS()
class CONCORD_API UConcordTransformerMapInt : public UConcordTransformerMap
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Map")
    TMap<int32, int32> Map;

    UPROPERTY(EditAnywhere, Category = "Map")
    int32 DefaultValue;

    FText GetDisplayName() const override { return INVTEXT("Map (Int)"); }
    FText GetTooltip() const override { return INVTEXT("Statically maps input integers to output integers."); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& InMultiIndex) const override
    {
        TArray<FConcordSharedExpression> SourceExpressions { GetConnectedTransformers()[0]->GetExpression(InMultiIndex) };
        return MakeShared<const FConcordMapExpression<int32>>(MoveTemp(SourceExpressions), SharedMap.ToSharedRef(), DefaultValue);
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        SharedMap = MakeShared<const TMap<int32, int32>>(Map);
        return { GetConnectedTransformers()[0]->GetShape(), EConcordValueType::Int };
    }

    TSharedPtr<const TMap<int32, int32>> SharedMap;
};

UCLASS()
class CONCORD_API UConcordTransformerMapFloat : public UConcordTransformerMap
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Map")
    TMap<int32, float> Map;

    UPROPERTY(EditAnywhere, Category = "Map")
    float DefaultValue;

    FText GetDisplayName() const override { return INVTEXT("Map (Float)"); }
    FText GetTooltip() const override { return INVTEXT("Statically maps input integers to output floats."); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& InMultiIndex) const override
    {
        TArray<FConcordSharedExpression> SourceExpressions { GetConnectedTransformers()[0]->GetExpression(InMultiIndex) };
        return MakeShared<const FConcordMapExpression<float>>(MoveTemp(SourceExpressions), SharedMap.ToSharedRef(), DefaultValue);
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        SharedMap = MakeShared<const TMap<int32, float>>(Map);
        return { GetConnectedTransformers()[0]->GetShape(), EConcordValueType::Float };
    }

    TSharedPtr<const TMap<int32, float>> SharedMap;
};
