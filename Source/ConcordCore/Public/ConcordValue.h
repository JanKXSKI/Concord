// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordFunction.h"
#include "ConcordValue.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogConcordCore, Log, All);

UENUM()
enum class EConcordValueType : uint8
{
    Int,
    Float,
    Error   UMETA(Hidden)
};

template<typename FValue> EConcordValueType GetConcordValueType();
template<> inline EConcordValueType GetConcordValueType<int32>() { return EConcordValueType::Int; }
template<> inline EConcordValueType GetConcordValueType<float>() { return EConcordValueType::Float; }

union FConcordValue
{
    int32 Int;
    float Float;

    FConcordValue(int32 InInt) : Int(InInt) {}
    FConcordValue(float InFloat) : Float(InFloat) {}
    FConcordValue(double InDouble) : Float(InDouble) {} // math functions return doubles when called with ints

    template<typename FValue> FValue& Get();
    template<> int32& Get<int32>() { return Int; }
    template<> float& Get<float>() { return Float; }
};


// for nativization
#if WITH_EDITOR
namespace Concord
{
    template<typename FValue> const TCHAR* ExpressionVariableString();
    template<> inline const TCHAR* ExpressionVariableString<int32>() { return TEXT("$int"); }
    template<> inline const TCHAR* ExpressionVariableString<float>() { return TEXT("$float"); }
    inline const TCHAR* NativeTypeString(EConcordValueType Type)
    {
        switch (Type)
        {
        case EConcordValueType::Int: return TEXT("int32");
        case EConcordValueType::Float: return TEXT("FFloatType");
        default: checkNoEntry(); return TEXT("int32");
        }
    }
    template<typename FValue> const TCHAR* NativeTypeString() { return NativeTypeString(GetConcordValueType<FValue>()); }
    template<typename FValue> const TCHAR* CapitalizedTypeString();
    template<> inline const TCHAR* CapitalizedTypeString<int32>() { return TEXT("Int"); }
    template<> inline const TCHAR* CapitalizedTypeString<float>() { return TEXT("Float"); }
    inline const TCHAR* EnumString(EConcordValueType Type)
    {
        switch (Type)
        {
        case EConcordValueType::Int: return TEXT("EConcordValueType::Int");
        case EConcordValueType::Float: return TEXT("EConcordValueType::Float");
        default: checkNoEntry(); return TEXT("EConcordValueType::Error");
        }
    }
}

#define CONCORD_STR(X) #X
#define CONCORD_XSTR(X) TEXT(CONCORD_STR(X))
#endif
