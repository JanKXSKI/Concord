// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerUnaryOperators.generated.h"

#define CONCORD_Not(V) V == 0
#define CONCORD_Abs(V) abs(V)
#define CONCORD_Acos(V) acos(V)
#define CONCORD_Asin(V) asin(V)
#define CONCORD_Atan(V) atan(V)
#define CONCORD_Cos(V) cos(V)
#define CONCORD_Cosh(V) cosh(V)
#define CONCORD_Erf(V) erf(V)
#define CONCORD_Exp(V) exp(V)
#define CONCORD_Log(V) log(V)
#define CONCORD_Log2(V) log2(V)
#define CONCORD_Log10(V) log10(V)
#define CONCORD_Sin(V) sin(V)
#define CONCORD_Sinh(V) sinh(V)
#define CONCORD_Sqrt(V) sqrt(V)
#define CONCORD_Tan(V) tan(V)
#define CONCORD_Tanh(V) tanh(V)

#if WITH_EDITOR
#define CONCORD_UNARY_OPERATOR_TO_STRING(Name) FString ToString() const override { return TEXT("\n        return ") + FString::Printf(CONCORD_XSTR(CONCORD_##Name(%s0)), Concord::ExpressionVariableString<FValue>()) + TEXT(";\n"); }
#else
#define CONCORD_UNARY_OPERATOR_TO_STRING(Name)  
#endif

#define CONCORD_TRANSFORMER_UNARY_OPERATOR_FULL(Name, DisplayName, NodeName, VertexType)\
    FText GetDisplayName() const override { return INVTEXT(DisplayName); }\
    FText GetNodeDisplayName() const override { return INVTEXT(NodeName); }\
\
    template<typename FValue>\
    class FConcordOperator##Name##Expression : public FConcordComputingExpression\
    {\
    public:\
        FConcordOperator##Name##Expression(FConcordSharedExpression&& InSourceExpression)\
            : FConcordComputingExpression({ MoveTemp(InSourceExpression) })\
        {}\
    \
        FConcordValue ComputeValue(const FConcordExpressionContext<float>& Context) const override\
        {\
            return CONCORD_##Name(SourceExpressions[0]->ComputeValue(Context).Get<FValue>());\
        }\
        CONCORD_UNARY_OPERATOR_TO_STRING(Name)\
    };\
\
    FConcordSharedExpression GetExpression(const FConcordMultiIndex& MultiIndex) const override\
    {\
        switch (GetConnectedType())\
        {\
        case EConcordValueType::Int: return MakeShared<const FConcordOperator##Name##Expression<int32>>(GetConnectedTransformers()[0]->GetExpression(MultiIndex));\
        case EConcordValueType::Float: return MakeShared<const FConcordOperator##Name##Expression<float>>(GetConnectedTransformers()[0]->GetExpression(MultiIndex));\
        default: checkNoEntry(); return MakeShared<const FConcordValueExpression<int32>>(0);\
        }\
    }\
private:\
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override\
    {\
        return { GetConnectedTransformers()[0]->GetShape(), VertexType };\
    }
#define CONCORD_TRANSFORMER_UNARY_OPERATOR(Name) CONCORD_TRANSFORMER_UNARY_OPERATOR_FULL(Name, #Name, #Name, GetConnectedType())


UCLASS(Abstract)
class CONCORD_API UConcordTransformerUnaryOperator : public UConcordTransformer
{
    GENERATED_BODY()
public:
    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, {}, {}, "0"}
        };
    }

    FString GetCategory() const override { return TEXT("Operator"); }
    FText GetTooltip() const override { return INVTEXT("Computes a value from each value in the input."); }

    EConcordValueType GetConnectedType() const { return GetConnectedTransformers()[0]->GetType(); }
};

UCLASS()
class CONCORD_API UConcordTransformerNot : public UConcordTransformerUnaryOperator
{
    GENERATED_BODY()
public:
    CONCORD_TRANSFORMER_UNARY_OPERATOR_FULL(Not, "Not", "!", EConcordValueType::Int)
};

UCLASS()
class CONCORD_API UConcordTransformerAbs : public UConcordTransformerUnaryOperator
{
    GENERATED_BODY()
public:
    CONCORD_TRANSFORMER_UNARY_OPERATOR(Abs)
};

UCLASS() class CONCORD_API UConcordTransformerAcos : public UConcordTransformerUnaryOperator { GENERATED_BODY() public: CONCORD_TRANSFORMER_UNARY_OPERATOR_FULL(Acos, "Acos", "Acos", EConcordValueType::Float) };
UCLASS() class CONCORD_API UConcordTransformerAsin : public UConcordTransformerUnaryOperator { GENERATED_BODY() public: CONCORD_TRANSFORMER_UNARY_OPERATOR_FULL(Asin, "Asin", "Asin", EConcordValueType::Float) };
UCLASS() class CONCORD_API UConcordTransformerAtan : public UConcordTransformerUnaryOperator { GENERATED_BODY() public: CONCORD_TRANSFORMER_UNARY_OPERATOR_FULL(Atan, "Atan", "Atan", EConcordValueType::Float) };
UCLASS() class CONCORD_API UConcordTransformerCos : public UConcordTransformerUnaryOperator { GENERATED_BODY() public: CONCORD_TRANSFORMER_UNARY_OPERATOR_FULL(Cos, "Cos", "Cos", EConcordValueType::Float) };
UCLASS() class CONCORD_API UConcordTransformerCosh : public UConcordTransformerUnaryOperator { GENERATED_BODY() public: CONCORD_TRANSFORMER_UNARY_OPERATOR_FULL(Cosh, "Cosh", "Cosh", EConcordValueType::Float) };
UCLASS() class CONCORD_API UConcordTransformerErf : public UConcordTransformerUnaryOperator { GENERATED_BODY() public: CONCORD_TRANSFORMER_UNARY_OPERATOR_FULL(Erf, "Erf", "Erf", EConcordValueType::Float) };
UCLASS() class CONCORD_API UConcordTransformerExp : public UConcordTransformerUnaryOperator { GENERATED_BODY() public: CONCORD_TRANSFORMER_UNARY_OPERATOR_FULL(Exp, "Exp", "Exp", EConcordValueType::Float) };
UCLASS() class CONCORD_API UConcordTransformerLog : public UConcordTransformerUnaryOperator { GENERATED_BODY() public: CONCORD_TRANSFORMER_UNARY_OPERATOR_FULL(Log, "Log", "Log", EConcordValueType::Float) };
UCLASS() class CONCORD_API UConcordTransformerLog2 : public UConcordTransformerUnaryOperator { GENERATED_BODY() public: CONCORD_TRANSFORMER_UNARY_OPERATOR_FULL(Log2, "Log2", "Log2", EConcordValueType::Float) };
UCLASS() class CONCORD_API UConcordTransformerLog10 : public UConcordTransformerUnaryOperator { GENERATED_BODY() public: CONCORD_TRANSFORMER_UNARY_OPERATOR_FULL(Log10, "Log10", "Log10", EConcordValueType::Float) };
UCLASS() class CONCORD_API UConcordTransformerSin : public UConcordTransformerUnaryOperator { GENERATED_BODY() public: CONCORD_TRANSFORMER_UNARY_OPERATOR_FULL(Sin, "Sin", "Sin", EConcordValueType::Float) };
UCLASS() class CONCORD_API UConcordTransformerSinh : public UConcordTransformerUnaryOperator { GENERATED_BODY() public: CONCORD_TRANSFORMER_UNARY_OPERATOR_FULL(Sinh, "Sinh", "Sinh", EConcordValueType::Float) };
UCLASS() class CONCORD_API UConcordTransformerSqrt : public UConcordTransformerUnaryOperator { GENERATED_BODY() public: CONCORD_TRANSFORMER_UNARY_OPERATOR_FULL(Sqrt, "Sqrt", "Sqrt", EConcordValueType::Float) };
UCLASS() class CONCORD_API UConcordTransformerTan : public UConcordTransformerUnaryOperator { GENERATED_BODY() public: CONCORD_TRANSFORMER_UNARY_OPERATOR_FULL(Tan, "Tan", "Tan", EConcordValueType::Float) };
UCLASS() class CONCORD_API UConcordTransformerTanh : public UConcordTransformerUnaryOperator { GENERATED_BODY() public: CONCORD_TRANSFORMER_UNARY_OPERATOR_FULL(Tanh, "Tanh", "Tanh", EConcordValueType::Float) };
