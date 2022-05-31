// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordError.h"
#include "ConcordShape.h"
#include "ConcordExpression.h"
#include "FactorGraph/ConcordFactorGraph.h"
#include "UObject/StrongObjectPtr.h"

DECLARE_LOG_CATEGORY_EXTERN(LogConcordCompiler, Log, All);

class UConcordModel;
class UConcordTransformer;
class UConcordVertex;
class UConcordBox;
class UConcordComposite;
class UConcordInstance;
class UConcordInstanceOutput;
class UConcordParameter;
class UConcordEmission;

class CONCORD_API FConcordCompiler
{
public:
    struct FResult
    {
        FResult();
        TSharedRef<FConcordFactorGraph<float>> FactorGraph;
        TOptional<FConcordError> Error;
    };

    static FResult Compile(const UConcordModel* Model, EConcordCycleMode CycleMode = EConcordCycleMode::Merge);
    const TMap<const UConcordModel*, TSharedRef<FConcordFactorGraph<float>>>& GetCompiledModels() const { return CompiledModels; }
private:
    FConcordCompiler();
    TOptional<FConcordError> CompileCompositeModel(const TStrongObjectPtr<const UConcordModel>& CompositeModel, FConcordFactorGraph<float>& FactorGraph);
    TOptional<FConcordError> SetupBox(const TStrongObjectPtr<const UConcordModel>& CompositeModel, const FName& BoxName, UConcordBox* Box, FConcordFactorGraph<float>& FactorGraph);
    TOptional<FConcordError> SetupComposite(const FName& CompositeName, UConcordComposite* Composite, TArray<TStrongObjectPtr<const UConcordModel>>& OutCompositeModels, FConcordFactorGraph<float>& FactorGraph);
    TOptional<FConcordError> SetupInstance(const FName& InstanceName, UConcordInstance* Instance, FConcordFactorGraph<float>& FactorGraph);
    TOptional<FConcordError> SetupParameter(const FName& ParameterName, UConcordParameter* Parameter, FConcordFactorGraph<float>& FactorGraph);
    TOptional<FConcordError> SetupEmission(const TStrongObjectPtr<const UConcordModel>& CompositeModel, UConcordEmission* Emission, FConcordFactorGraph<float>& FactorGraph);
    FConcordError MakeCompositeError(const FConcordError& Error) const;
    TOptional<FConcordError> SetDisjointSubgraphRootFlatRandomVariableIndices(FConcordFactorGraph<float>& FactorGraph) const;
    void AddConnected(int32 FlatRandomVariableIndex, TSet<int32>& AddedFlatRandomVariableIndices, const FConcordFactorGraph<float>& FactorGraph) const;
    TOptional<FConcordError> HandleCycles(EConcordCycleMode CycleMode, FConcordFactorGraph<float>& FactorGraph) const;
    bool MergeCycle(int32 FlatRandomVariableIndex, const FConcordHandle* Handle, TArray<int32>& RandomVariablePath, TArray<const FConcordHandle*>& FactorPath, FConcordFactorGraph<float>& FactorGraph) const;
    static FConcordError GetCycleError(const TArray<int32>& RandomVariableCycle, FConcordFactorGraph<float>& FactorGraph);
    static FString GetRandomVariableIndexString(int32 FlatRandomVariableIndex, FConcordFactorGraph<float>& FactorGraph);

    TArray<const UConcordModel*> ModelPath;
    FString CompositePrefix;
    int32 VariationBlockOffset;
    int32 IntParameterBlockOffset;
    int32 FloatParameterBlockOffset;
    TArray<FConcordSharedRandomVariableExpression> AllRandomVariableExpressions;
    TArray<FConcordSharedParameterExpression<int32>> AllIntParameterExpressions;
    TArray<FConcordSharedParameterExpression<float>> AllFloatParameterExpressions;
    TSet<UConcordVertex*> VisitedVertices;
    TMap<const UConcordModel*, TSharedRef<FConcordFactorGraph<float>>> CompiledModels;

    FConcordSharedExpression GetRandomVariableExpression(const FConcordMultiIndex& BoxLocalIndex, FConcordShape BoxShape, int32 VariationBlockOffset) const;
    template<typename FValue> FConcordSharedExpression GetParameterExpression(const FConcordMultiIndex& ParameterLocalIndex, FConcordShape ParameterShape, int32 ParameterBlockOffset) const;

    template<typename FValue> int32& GetParameterBlockOffset();
    template<> int32& GetParameterBlockOffset<int32>() { return IntParameterBlockOffset; }
    template<> int32& GetParameterBlockOffset<float>() { return FloatParameterBlockOffset; }

    template<typename FValue> TArray<FConcordSharedParameterExpression<FValue>>& GetAllParameterExpressions();
    template<> TArray<FConcordSharedParameterExpression<int32>>& GetAllParameterExpressions<int32>() { return AllIntParameterExpressions; }
    template<> TArray<FConcordSharedParameterExpression<float>>& GetAllParameterExpressions<float>() { return AllFloatParameterExpressions; }
    template<typename FValue> const TArray<FConcordSharedParameterExpression<FValue>>& GetAllParameterExpressions() const;
    template<> const TArray<FConcordSharedParameterExpression<int32>>& GetAllParameterExpressions<int32>() const { return AllIntParameterExpressions; }
    template<> const TArray<FConcordSharedParameterExpression<float>>& GetAllParameterExpressions<float>() const { return AllFloatParameterExpressions; }

    template<typename FValue> void AddParameter(const FName& ParameterName, UConcordParameter* Parameter, FConcordFactorGraph<float>& FactorGraph);
    template<typename FValue> void AddTargetParameter(const FName& TargetParameterName, UConcordInstanceOutput* ObservedInstanceOutput, FConcordFactorGraph<float>& FactorGraph);
    void AddEmissionParameter(const FName& EmissionParameterName, int32 Size, FConcordFactorGraph<float>& FactorGraph, TArray<FConcordSharedExpression>& OutParameterExpressions);
    void AddHandle(const FConcordSharedExpression& FactorExpression, FConcordFactorGraph<float>& FactorGraph) const;
};
