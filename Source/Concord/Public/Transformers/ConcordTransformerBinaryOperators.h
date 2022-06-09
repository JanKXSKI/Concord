// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerBinaryOperators.generated.h"

#define CONCORD_Add(X) X##0 + X##1
#define CONCORD_Subtract(X) X##0 - X##1
#define CONCORD_Multiply(X) X##0 * X##1
#define CONCORD_Divide(X) X##0 / X##1
#define CONCORD_Modulo(X) ConcordModulo(X##0, X##1)
#define CONCORD_Min(X) min(X##0, X##1)
#define CONCORD_Max(X) max(X##0, X##1)
#define CONCORD_Equal(X) X##0 == X##1
#define CONCORD_NotEqual(X) X##0 != X##1
#define CONCORD_LessThan(X) X##0 < X##1
#define CONCORD_LessThanOrEqual(X) X##0 <= X##1
#define CONCORD_GreaterThan(X) X##0 > X##1
#define CONCORD_GreaterThanOrEqual(X) X##0 >= X##1
#define CONCORD_And(X) X##0 && X##1
#define CONCORD_Or(X) X##0 || X##1
#define CONCORD_Coalesce(X) ConcordCoalesce(X##0, X##1)
#define CONCORD_Pow(X) pow(X##0, X##1)
#define CONCORD_Atan2(X) atan2(X##0, X##1)
#define CONCORD_Hypot(X) hypot(X##0, X##1)

#if WITH_EDITOR
#define CONCORD_BINARY_OPERATOR_TO_STRING(Name) FString ToString() const override { return FString::Format(TEXT("\n        return ") CONCORD_XSTR(CONCORD_##Name({0})) TEXT(";\n"), { Concord::ExpressionVariableString<FValue>() }); }
#else
#define CONCORD_BINARY_OPERATOR_TO_STRING(Name)  
#endif

#define CONCORD_TRANSFORMER_BINARY_OPERATOR_FULL(Name, DisplayName, NodeName, VertexType)\
    FText GetDisplayName() const override { return FText::AsCultureInvariant(DisplayName); }\
    FText GetNodeDisplayName() const override { return FText::AsCultureInvariant(NodeName); }\
\
    template<typename FValue>\
    class FConcordOperator##Name##Expression : public FConcordComputingExpression\
    {\
    public:\
        FConcordOperator##Name##Expression(TArray<FConcordSharedExpression>&& InSourceExpressions): FConcordComputingExpression(MoveTemp(InSourceExpressions)) {}\
        FConcordValue ComputeValue(const FConcordExpressionContext<float>& Context) const override\
        {\
            const FValue V0 = SourceExpressions[0]->ComputeValue(Context).Get<FValue>();\
            const FValue V1 = SourceExpressions[1]->ComputeValue(Context).Get<FValue>();\
            return CONCORD_##Name(V);\
        }\
        CONCORD_BINARY_OPERATOR_TO_STRING(Name)\
    };\
\
    FConcordSharedExpression GetExpression(const FConcordMultiIndex& MultiIndex) const override\
    {\
        const EConcordValueType SumType = GetSumType();\
        switch (SumType)\
        {\
        case EConcordValueType::Int: return MakeShared<const FConcordOperator##Name##Expression<int32>>(GetSourceExpressions(MultiIndex, EConcordValueType::Int));\
        case EConcordValueType::Float: return MakeShared<const FConcordOperator##Name##Expression<float>>(GetSourceExpressions(MultiIndex, EConcordValueType::Float));\
        default: checkNoEntry(); return MakeShared<const FConcordValueExpression<int32>>(0);\
        }\
    }\
private:\
    EConcordValueType GetVertexType() const override { return VertexType; }
#define CONCORD_TRANSFORMER_BINARY_OPERATOR(Name, NodeName) CONCORD_TRANSFORMER_BINARY_OPERATOR_FULL(Name, #Name, NodeName, GetSumType())

UCLASS(Abstract)
class CONCORD_API UConcordTransformerBinaryOperator : public UConcordTransformer
{
    GENERATED_BODY()
public:
    UConcordTransformerBinaryOperator()
        : OuterDimensionIndex(-1)
        , ActualOuterDimensionIndex(0)
    {}

    UPROPERTY(EditAnywhere, Category = "Binary Operator")
    bool bOuter;

    UPROPERTY(EditAnywhere, Category = "Binary Operator", meta = (EditCondition = "bOuter"))
    int32 OuterDimensionIndex;

    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, {}, {}, "0" },
            { {}, {}, {}, "0" }
        };
    }

    FString GetCategory() const override { return TEXT("Operator"); }
    FText GetTooltip() const override { return INVTEXT("Broadcasts its inputs and computes a value from each resulting pair."); }
protected:
    TArray<FConcordSharedExpression> GetSourceExpressions(const FConcordMultiIndex& MultiIndex, EConcordValueType SumType) const
    {
        if (!bOuter) return GetBroadcastedConnectedExpressions(MultiIndex, SumType);
        TArray<FConcordSharedExpression> SourceExpressions; SourceExpressions.Reserve(2);
        for (int32 Index = 0; Index < 2; ++Index)
        {
            FConcordMultiIndex OuterMultiIndex = MultiIndex;
            OuterMultiIndex.RemoveAt(ActualOuterDimensionIndex + 1 - Index, 1, false);
            ConcordShape::Unbroadcast(OuterMultiIndex, GetConnectedTransformers()[Index]->GetShape());
            SourceExpressions.Add(GetConnectedExpression(GetConnectedTransformers()[Index], OuterMultiIndex, SumType));
        }
        return MoveTemp(SourceExpressions);
    }
private:
    virtual EConcordValueType GetVertexType() const PURE_VIRTUAL(UConcordTransformerBinaryOperator::GetVertexType, return EConcordValueType::Error;)

    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        if (!bOuter) return { ComputeBroadcastedShape(OutErrorMessage), GetVertexType() };
        TArray<FConcordShape> ConnectedShapes = GetConnectedShapes();
        const int32 MaxNum = FMath::Max(ConnectedShapes[0].Num(), ConnectedShapes[1].Num());
        for (FConcordShape& ConnectedShape : ConnectedShapes) while (ConnectedShape.Num() < MaxNum) ConnectedShape.Add(1);
        if (!GetActualDimensionIndex(OuterDimensionIndex, ConnectedShapes[0], ActualOuterDimensionIndex, OutErrorMessage)) return {};
        int32 OuterSizes[2];
        for (int32 Index = 0; Index < 2; ++Index)
        {
            OuterSizes[Index] = ConnectedShapes[Index][ActualOuterDimensionIndex];
            ConnectedShapes[Index].RemoveAt(ActualOuterDimensionIndex, 1, false);
        }
        TOptional<FConcordShape> OptionalBroadcastedShape = ConcordShape::Broadcast(ConnectedShapes);
        if (!OptionalBroadcastedShape) { OutErrorMessage = TEXT("Input shapes could not be broadcasted together."); return {}; }
        FConcordShape& BroadcastedShape = OptionalBroadcastedShape.GetValue();
        BroadcastedShape.Insert(OuterSizes, 2, ActualOuterDimensionIndex);
        return { MoveTemp(BroadcastedShape), GetVertexType() };
    }

    int32 ActualOuterDimensionIndex;
};

UCLASS()
class CONCORD_API UConcordTransformerAdd : public UConcordTransformerBinaryOperator
{
    GENERATED_BODY()
public:
    CONCORD_TRANSFORMER_BINARY_OPERATOR(Add, "+")
};

UCLASS()
class CONCORD_API UConcordTransformerSubtract : public UConcordTransformerBinaryOperator
{
    GENERATED_BODY()
public:
    CONCORD_TRANSFORMER_BINARY_OPERATOR(Subtract, "-")
};

UCLASS()
class CONCORD_API UConcordTransformerMultiply : public UConcordTransformerBinaryOperator
{
    GENERATED_BODY()
public:
    CONCORD_TRANSFORMER_BINARY_OPERATOR(Multiply, UTF8_TO_TCHAR("\u00D7"))
};

UCLASS()
class CONCORD_API UConcordTransformerDivide : public UConcordTransformerBinaryOperator
{
    GENERATED_BODY()
public:
    CONCORD_TRANSFORMER_BINARY_OPERATOR(Divide, "/")
};

UCLASS()
class CONCORD_API UConcordTransformerModulo : public UConcordTransformerBinaryOperator
{
    GENERATED_BODY()
public:
    CONCORD_TRANSFORMER_BINARY_OPERATOR(Modulo, "%")
};

UCLASS()
class CONCORD_API UConcordTransformerMin : public UConcordTransformerBinaryOperator
{
    GENERATED_BODY()
public:
    CONCORD_TRANSFORMER_BINARY_OPERATOR(Min, "Min")
};

UCLASS()
class CONCORD_API UConcordTransformerMax : public UConcordTransformerBinaryOperator
{
    GENERATED_BODY()
public:
    CONCORD_TRANSFORMER_BINARY_OPERATOR(Max, "Max")
};

UCLASS()
class CONCORD_API UConcordTransformerEqual : public UConcordTransformerBinaryOperator
{
    GENERATED_BODY()
public:
    CONCORD_TRANSFORMER_BINARY_OPERATOR(Equal, "=")
};

UCLASS()
class CONCORD_API UConcordTransformerNotEqual : public UConcordTransformerBinaryOperator
{
    GENERATED_BODY()
public:
    CONCORD_TRANSFORMER_BINARY_OPERATOR_FULL(NotEqual, "Not Equal", UTF8_TO_TCHAR("\u2260"), EConcordValueType::Int)
};

UCLASS()
class CONCORD_API UConcordTransformerLessThan : public UConcordTransformerBinaryOperator
{
    GENERATED_BODY()
public:
    CONCORD_TRANSFORMER_BINARY_OPERATOR_FULL(LessThan, "Less Than", UTF8_TO_TCHAR("\u003C"), EConcordValueType::Int)
};

UCLASS()
class CONCORD_API UConcordTransformerLessThanOrEqual : public UConcordTransformerBinaryOperator
{
    GENERATED_BODY()
public:
    CONCORD_TRANSFORMER_BINARY_OPERATOR_FULL(LessThanOrEqual, "Less Than Or Equal", UTF8_TO_TCHAR("\u2264"), EConcordValueType::Int)
};

UCLASS()
class CONCORD_API UConcordTransformerGreaterThan : public UConcordTransformerBinaryOperator
{
    GENERATED_BODY()
public:
    CONCORD_TRANSFORMER_BINARY_OPERATOR_FULL(GreaterThan, "Greater Than", UTF8_TO_TCHAR("\u003E"), EConcordValueType::Int)
};

UCLASS()
class CONCORD_API UConcordTransformerGreaterThanOrEqual : public UConcordTransformerBinaryOperator
{
    GENERATED_BODY()
public:
    CONCORD_TRANSFORMER_BINARY_OPERATOR_FULL(GreaterThanOrEqual, "Greater Than Or Equal", UTF8_TO_TCHAR("\u2265"), EConcordValueType::Int)
};

UCLASS()
class CONCORD_API UConcordTransformerAnd : public UConcordTransformerBinaryOperator
{
    GENERATED_BODY()
public:
    CONCORD_TRANSFORMER_BINARY_OPERATOR_FULL(And, "And", "&&", EConcordValueType::Int)
};

UCLASS()
class CONCORD_API UConcordTransformerOr : public UConcordTransformerBinaryOperator
{
    GENERATED_BODY()
public:
    CONCORD_TRANSFORMER_BINARY_OPERATOR_FULL(Or, "Or", "||", EConcordValueType::Int)
};

UCLASS()
class CONCORD_API UConcordTransformerCoalesce : public UConcordTransformerBinaryOperator
{
    GENERATED_BODY()
public:
    FText GetTooltip() const override { return INVTEXT("The output equals the first operand if it is non-zero, otherwise it's the second operand."); }
    CONCORD_TRANSFORMER_BINARY_OPERATOR(Coalesce, "??")
};

UCLASS()
class CONCORD_API UConcordTransformerPow : public UConcordTransformerBinaryOperator
{
    GENERATED_BODY()
public:
    CONCORD_TRANSFORMER_BINARY_OPERATOR_FULL(Pow, "Pow", "Pow", EConcordValueType::Float)
};

UCLASS()
class CONCORD_API UConcordTransformerAtan2 : public UConcordTransformerBinaryOperator
{
    GENERATED_BODY()
public:
    CONCORD_TRANSFORMER_BINARY_OPERATOR_FULL(Atan2, "Atan2", "Atan2", EConcordValueType::Float)
};

UCLASS()
class CONCORD_API UConcordTransformerHypot : public UConcordTransformerBinaryOperator
{
    GENERATED_BODY()
public:
    FText GetTooltip() const override { return INVTEXT("Computes sqrt(x^2 + y^2)."); }
    CONCORD_TRANSFORMER_BINARY_OPERATOR_FULL(Hypot, "Hypot", "Hypot", EConcordValueType::Float)
};
