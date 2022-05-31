 // Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerReductions.generated.h"

#define CONCORD_Sum(V) Acc += V
#define CONCORD_Product(V) Acc *= V
#define CONCORD_Minimum(V) Acc = min(Acc, V)
#define CONCORD_Maximum(V) Acc = max(Acc, V)
#define CONCORD_CountNonzero(V) Acc += (V == 0) ? 1 : 0
#define CONCORD_Any(V) Acc = Acc || (V != 0)
#define CONCORD_All(V) Acc = Acc && (V != 0)

#if WITH_EDITOR
#define CONCORD_REDUCTION_TO_STRING(Name, FDefaultValueType, DefaultValue) \
    virtual FString ToString() const override\
    {\
        const FString UpdateString = FString::Printf(CONCORD_XSTR(CONCORD_##Name(%s0array(Index))), Concord::ExpressionVariableString<FValue>());\
        return FString::Printf(TEXT("\n        %s Acc = %i; for (int32 Index = 0; Index < %i; ++Index) %s; return Acc;\n"), Concord::NativeTypeString<FDefaultValueType>(), DefaultValue, SourceExpressions.Num(), *UpdateString);\
    }
#else
#define CONCORD_REDUCTION_TO_STRING(Name, FDefaultValueType, DefaultValue)  
#endif

#define CONCORD_TRANSFORMER_REDUCTION_BODY_FULL(Name, DisplayName, VertexType, FDefaultValueType, DefaultValue)\
FText GetDisplayName() const override { return INVTEXT(DisplayName); }\
\
template<typename FValue>\
class FConcord##Name##ReductionExpression : public FConcordComputingExpression\
{\
public:\
    FConcord##Name##ReductionExpression(TArray<FConcordSharedExpression>&& InSourceExpressions)\
        : FConcordComputingExpression(MoveTemp(InSourceExpressions))\
    {}\
\
    virtual FConcordValue ComputeValue(const FConcordExpressionContext<float>& Context) const override\
    {\
        FDefaultValueType Acc = DefaultValue;\
        for (const FConcordSharedExpression& SourceExpression : SourceExpressions)\
            CONCORD_##Name(SourceExpression->ComputeValue(Context).Get<FValue>());\
        return Acc;\
    }\
    CONCORD_REDUCTION_TO_STRING(Name, FDefaultValueType, DefaultValue)\
};\
\
FConcordSharedExpression GetExpression(TArray<FConcordSharedExpression>&& InSourceExpressions) const override\
{\
    switch (GetConnectedTransformers()[0]->GetType())\
    {\
    case EConcordValueType::Int: return MakeShared<const FConcord##Name##ReductionExpression<int32>>(MoveTemp(InSourceExpressions));\
    case EConcordValueType::Float: return MakeShared<const FConcord##Name##ReductionExpression<float>>(MoveTemp(InSourceExpressions));\
    default: checkNoEntry(); return MakeShared<const FConcordValueExpression<int32>>(0);\
    }\
}\
FSetupResult Setup(TOptional<FString>& OutErrorMessage) override\
{\
    return { ComputeShape(OutErrorMessage), VertexType };\
}
#define CONCORD_TRANSFORMER_REDUCTION_BODY(Name, DefaultValue) CONCORD_TRANSFORMER_REDUCTION_BODY_FULL(Name, #Name, GetConnectedTransformers()[0]->GetType(), FValue, DefaultValue)


UCLASS(Abstract)
class CONCORD_API UConcordTransformerReduction : public UConcordTransformer
{
    GENERATED_BODY()
public:
    UConcordTransformerReduction() : DimensionIndex(0)
    {}

    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, {}, {}, "0"}
        };
    }

    UPROPERTY(EditAnywhere, Category = "Reduction")
    int32 DimensionIndex;

    UPROPERTY(EditAnywhere, Category = "Reduction")
    bool bCumulative;

    UPROPERTY(EditAnywhere, Category = "Reduction", meta=(EditCondition="bCumulative"))
    bool bStartAtDefault;

    FString GetCategory() const override { return TEXT("Reduction"); }
    FText GetTooltip() const override { return INVTEXT("Computes a reduction of the input along the specified dimension."); }

    virtual FConcordSharedExpression GetExpression(TArray<FConcordSharedExpression>&& InSourceExpressions) const
    {
        checkNoEntry();
        return MakeShared<const FConcordValueExpression<int32>>(0);
    }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& MultiIndex) const override
    {
        if (bCumulative)
        {
            const int32 ExpressionsNum = MultiIndex[ActualDimensionIndex] + int32(!bStartAtDefault);
            TArray<FConcordSharedExpression> Expressions; Expressions.Reserve(ExpressionsNum);
            FConcordMultiIndex CumulativeMultiIndex = MultiIndex;
            for (CumulativeMultiIndex[ActualDimensionIndex] = 0; CumulativeMultiIndex[ActualDimensionIndex] < ExpressionsNum; ++CumulativeMultiIndex[ActualDimensionIndex])
                Expressions.Add(GetConnectedTransformers()[0]->GetExpression(CumulativeMultiIndex));
            return GetExpression(MoveTemp(Expressions));
        }

        const int32 ReductionSize = GetConnectedTransformers()[0]->GetShape()[ActualDimensionIndex];
        TArray<FConcordSharedExpression> Expressions; Expressions.Reserve(ReductionSize);
        FConcordMultiIndex ReducedMultiIndex = MultiIndex;
        if (GetConnectedTransformers()[0]->GetShape().Num() == 1) ReducedMultiIndex[0] = 0;
        else ReducedMultiIndex.Insert(0, ActualDimensionIndex);
        for (; ReducedMultiIndex[ActualDimensionIndex] < ReductionSize; ++ReducedMultiIndex[ActualDimensionIndex])
            Expressions.Add(GetConnectedTransformers()[0]->GetExpression(ReducedMultiIndex));
        return GetExpression(MoveTemp(Expressions));
    }
protected:
    TArray<int32> ComputeShape(TOptional<FString>& OutErrorMessage)
    {
        const FConcordShape& ConnectedShape = GetConnectedTransformers()[0]->GetShape();
        if (!GetActualDimensionIndex(DimensionIndex, ConnectedShape, ActualDimensionIndex, OutErrorMessage)) return {};
        if (bCumulative) return ConnectedShape;
        else if (ConnectedShape.Num() == 1) return { 1 };
        else
        {
            TArray<int32> ReducedShape = ConnectedShape;
            ReducedShape.RemoveAt(ActualDimensionIndex);
            return ReducedShape;
        }
    }
private:
    int32 ActualDimensionIndex;
};

UCLASS()
class CONCORD_API UConcordTransformerSum : public UConcordTransformerReduction
{
    GENERATED_BODY()
public:
    CONCORD_TRANSFORMER_REDUCTION_BODY(Sum, 0)
};

UCLASS()
class CONCORD_API UConcordTransformerProduct : public UConcordTransformerReduction
{
    GENERATED_BODY()
public:
    CONCORD_TRANSFORMER_REDUCTION_BODY(Product, 1)
};

UCLASS()
class CONCORD_API UConcordTransformerMinimum : public UConcordTransformerReduction
{
    GENERATED_BODY()
public:
    CONCORD_TRANSFORMER_REDUCTION_BODY(Minimum, TNumericLimits<FValue>::Max())
};

UCLASS()
class CONCORD_API UConcordTransformerMaximum : public UConcordTransformerReduction
{
    GENERATED_BODY()
public:
    CONCORD_TRANSFORMER_REDUCTION_BODY(Maximum, TNumericLimits<FValue>::Lowest())
};

UCLASS()
class CONCORD_API UConcordTransformerCountNonzero : public UConcordTransformerReduction
{
    GENERATED_BODY()
public:
    CONCORD_TRANSFORMER_REDUCTION_BODY_FULL(CountNonzero, "Count Nonzero", EConcordValueType::Int, int32, 0)
};

UCLASS()
class CONCORD_API UConcordTransformerAny : public UConcordTransformerReduction
{
    GENERATED_BODY()
public:
    FText GetTooltip() const override { return INVTEXT("Checks if there are any elements in the input that are true (i.e. not equal 0)."); }
    CONCORD_TRANSFORMER_REDUCTION_BODY_FULL(Any, "Any", EConcordValueType::Int, int32, 0)
};

UCLASS()
class CONCORD_API UConcordTransformerAll : public UConcordTransformerReduction
{
    GENERATED_BODY()
public:
    FText GetTooltip() const override { return INVTEXT("Checks if all elements of the input are true (i.e. not equal 0)."); }
    CONCORD_TRANSFORMER_REDUCTION_BODY_FULL(All, "All", EConcordValueType::Int, int32, 1)
};
