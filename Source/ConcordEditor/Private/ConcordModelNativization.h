// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordModel.h"
#include "ConcordCompiler.h"
#include "ConcordError.h"
#include "ConcordExpression.h"
#include "ConcordFactorGraphDynamic.h"
#include "ConcordNativeModel.h"

class FConcordModelNativization
{
public:
    static UConcordNativeModel* CreateNativeModel(const UConcordModel* Model);
    static TOptional<FConcordError> SetupNativeModel(UConcordNativeModel* NativeModel, const UConcordModel* Model, EConcordCycleMode CycleMode, bool bLiveCoding);
private:
    static FString GetImplementationCode(const FString& ClassName, const FConcordFactorGraph<float>& FactorGraph);
};

struct FConcordFactorGraphTranspiler
{
    void Transpile();
    TArray<FString> GetFactorNames() const;
    TArray<FString> GetOutputNames() const;
private:
    friend class FConcordModelNativization;
    FConcordFactorGraphTranspiler(FString& InOut, const FString& InClassName, const FConcordFactorGraph<float>& InFactorGraph)
        : Out({ InOut, InClassName })
        , Buf({ BufStr, InClassName })
        , ClassName(InClassName)
        , FactorGraph(InFactorGraph)
        , CurrentIndex(0)
        , bCurrentIsOutput(false)
    { BufStr.Reserve(5000); }

    struct FOutput
    {
        FString& Out;
        const FString& ClassName;
        template<typename FStringType> FOutput& Write(const FStringType& String);
        FOutput& Write(const TCHAR* String, int32 Count);
        FOutput& Write(bool bCondition, const TCHAR* String);
        FOutput& Write(int32 Index);
        FOutput& WriteName(int32 Index);
        FOutput& WriteStructTop(int32 Index);
        FOutput& WriteEvalContext();
        FOutput& WriteTemplateEvalTop(const TCHAR* ReturnType, bool bIsArray = false);
        FOutput& WriteIndexCountEquals();
        template<typename FElem, typename FElemToString> FOutput& WriteArray(const TArray<FElem>& Array, const TCHAR* Open, const TCHAR* Combine, const TCHAR* Close, const FElemToString& ElemToStringFunc);
        FOutput& WriteArray(const TArray<int32>& Array);
        template<typename FIntToString> FOutput& WriteSequence(int32 Max, const TCHAR* Open, const TCHAR* Combine, const TCHAR* Close, const FIntToString& IntToStringFunc);
        FOutput& WriteStdArray(int32 Size, bool bConst = false, bool bReference = false);
        FOutput& Indent(int32 Level);
    } Out, Buf;

    FString BufStr;
    const FString& ClassName; 
    const FConcordFactorGraph<float>& FactorGraph;
    
    int32 CurrentIndex;
    TArray<int32> CurrentRootArgumentIndices;
    TArray<int32> CurrentRootNeighboringRandomVariableIndices;
    bool bCurrentIsOutput;

    TMap<FString, int32> ExpressionStringToIndex;
    struct FInstantiation { int32 InstanceIndex; TArray<int32> ArgumentIndices; };
    TMap<int32, TArray<FInstantiation>> TemplateInstantiations;
    TMap<int32, int32> RootHandleMap;
    struct FComposite { int32 CompositeIndex; TArray<int32> ElementIndices; bool bConstantElementIndices; int32 ElisionOffset, ElisionStride; };
    TArray<FComposite> MergedHandles, Outputs, Arrays;
    TMap<int32, int32> ArgumentIndicesCountMap;

    int32 TranspileHandle(const ConcordFactorGraphDynamic::FHandle* Handle);
    void WriteHandle(const FComposite& Composite);
    int32 TranspileOutput(const TPair<FName, TUniquePtr<FConcordFactorGraph<float>::FOutput>>& NameOutputPair);
    void WriteOutput(const FComposite& Composite, EConcordValueType Type);
    template<typename FExpression>
    TArray<int32> TranspileComposite(const TArray<FExpression>& Expressions, EConcordValueType ExpectedType);
    template<typename FExpression>
    TArray<int32> TranspileComposite(const TArrayView<FExpression>& Expressions, EConcordValueType ExpectedType);
    int32 TranspileExpression(const FConcordExpression* Expression, EConcordValueType ExpectedType);
    int32 TranspileExpression(const FConcordSharedExpression& Expression, EConcordValueType ExpectedType);
    template<typename FAtomicHandle> int32 TranspileExpression(const FAtomicHandle& AtomicHandle, EConcordValueType ExpectedType);
    
    bool GetIndex(const FString& ExpressionString, int32& IndexOut) const;
    int32 Transpile(const FConcordRandomVariableExpression* RandomVariableExpression);
    template<typename FValue> int32 Transpile(const FConcordParameterExpression<FValue>* ParameterExpression);
    int32 Transpile(const FConcordComputingExpression* ComputingExpression, EConcordValueType ExpectedType);
    template<typename FValue> int32 Transpile(const FConcordValueExpression<FValue>* ValueExpression);

    static bool IsConstant(const TArray<int32>& ElementIndices);
    void GetElisionInfoForConstantElementIndex(int32 ElementIndex, int32 CurrentRootArgumentIndicesOffset, int32& OutElisionOffset, int32& OutElisionStride);
    FComposite& FindOrAddComposite(TArray<FComposite>& Composites, TArray<int32>&& ElementIndices, bool& bOutAddedNewComposite, int32 CurrentRootArgumentIndicesOffset = 0);

    struct FExpressionVariable
    {
        int32 Begin;
        int32 End;
        EConcordValueType ExpectedType;
        int32 SourceExpressionIndex;
        int32 SourceExpressionArrayNum;
        int32 ArgIndex;
    };
    TMap<FString, TSharedPtr<TArray<FExpressionVariable>>> ParsedComputingExpressionStrings;
    TSharedPtr<TArray<FExpressionVariable>> GetComputingExpressionVariables(const FString& ExpressionString, EConcordValueType ExpectedType);

    int32 TranspileArray(const FConcordComputingExpression* ComputingExpression, const FExpressionVariable* ExpressionVariable);
    void WriteArray(const FComposite& Composite, EConcordValueType Type);
};
