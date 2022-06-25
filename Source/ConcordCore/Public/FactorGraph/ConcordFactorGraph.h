// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordFactorHandle.h"
#include "ConcordExpressionContext.h"
#include "ConcordValue.h"
#include "ConcordFactorGraph.generated.h"

class FConcordSampler;

UENUM()
enum class EConcordCycleMode : uint8
{
    Merge,
    Error,
    Ignore
};

USTRUCT()
struct FConcordFactorGraphBlock
{
    GENERATED_BODY()

    FConcordFactorGraphBlock() : Offset(0), Size(0), bLocal(false) {}
    FConcordFactorGraphBlock(int32 InOffset , int32 InSize, bool bInLocal = false) : Offset(InOffset), Size(InSize), bLocal(bInLocal) {}

    UPROPERTY()
    int32 Offset;

    UPROPERTY()
    int32 Size;

    UPROPERTY()
    bool bLocal;
};
using FConcordFactorGraphBlocks = TMap<FName, FConcordFactorGraphBlock>;

template<typename FFloatType>
class CONCORDCORE_API FConcordFactorGraph
{
public:
    FConcordFactorGraph() : bHasCycle(false) {}
    FConcordFactorGraph(const FConcordFactorGraph&) = delete;
    FConcordFactorGraph& operator=(const FConcordFactorGraph&) = delete;

    struct FOutput
    {
        FOutput(EConcordValueType InType) : Type(InType) {}
        virtual ~FOutput() {}
        virtual void Eval(const FConcordExpressionContext<FFloatType>& Context, const TArrayView<int32>& OutData) const { checkNoEntry(); }
        virtual void Eval(const FConcordExpressionContext<FFloatType>& Context, const TArrayView<FFloatType>& OutData) const { checkNoEntry(); }
        virtual int32 Num() const = 0;
        EConcordValueType GetType() const { return Type; }
    private:
        const EConcordValueType Type;
    };
    using FOutputs = TMap<FName, TUniquePtr<FOutput>>;

    const TArray<TUniquePtr<FConcordFactorHandleBase<FFloatType>>>& GetHandles() const { return Handles; }
    int32 GetRandomVariableCount() const { return RandomVariableStateCounts.Num(); }
    int32 GetStateCount(int32 FlatRandomVariableIndex) const { return RandomVariableStateCounts[FlatRandomVariableIndex]; }
    const TArray<const FConcordFactorHandleBase<FFloatType>*>& GetNeighboringHandles(int32 FlatRandomVariableIndex) const { return RandomVariableNeighboringHandles[FlatRandomVariableIndex]; }
    const TArray<int32>& GetDisjointSubgraphRootFlatRandomVariableIndices() const { return DisjointSubgraphRootFlatRandomVariableIndices; }
    const FOutputs& GetOutputs() const { return Outputs; }

    const FConcordFactorGraphBlocks& GetVariationBlocks() const { return VariationBlocks; }
    template<typename FValue> const FConcordFactorGraphBlocks& GetParameterBlocks() const;
    template<> inline const FConcordFactorGraphBlocks& GetParameterBlocks<int32>() const { return IntParameterBlocks; }
    template<> inline const FConcordFactorGraphBlocks& GetParameterBlocks<float>() const { return FloatParameterBlocks; }
    TArray<TPair<FName, FConcordFactorGraphBlock>> GetAllParameterBlocks() const;
    template<typename FValue> const TArray<FValue>& GetParameterDefaultValues() const;
    template<> inline const TArray<int32>& GetParameterDefaultValues() const { return IntParameterDefaultValues; }
    template<> inline const TArray<float>& GetParameterDefaultValues() const { return FloatParameterDefaultValues; }

#if WITH_EDITORONLY_DATA
    const TArray<FName>& GetTrainableFloatParameterBlockNames() const { return TrainableFloatParameterBlockNames; }
#endif
    using FInstanceSamplers = TArray<TPair<FName, TSharedRef<FConcordSampler>>>;
    const FInstanceSamplers& GetInstanceSamplers() const { return InstanceSamplers; }

    bool GetHasCycle() const { return bHasCycle; }
    uint64 GetComplexity() const;

private:
    TArray<TUniquePtr<FConcordFactorHandleBase<FFloatType>>> Handles;
    TArray<int32> RandomVariableStateCounts;
    TArray<TArray<const FConcordFactorHandleBase<FFloatType>*>> RandomVariableNeighboringHandles;
    TArray<int32> DisjointSubgraphRootFlatRandomVariableIndices;
    FOutputs Outputs;

    FConcordFactorGraphBlocks VariationBlocks;
    FConcordFactorGraphBlocks IntParameterBlocks;
    FConcordFactorGraphBlocks FloatParameterBlocks;
    TArray<int32> IntParameterDefaultValues;
    TArray<float> FloatParameterDefaultValues;
#if WITH_EDITORONLY_DATA
    TArray<FName> TrainableFloatParameterBlockNames;
#endif
    FInstanceSamplers InstanceSamplers;

    bool bHasCycle;

    void CompactAndShrink();

    template<typename FValue> FConcordFactorGraphBlocks& GetParameterBlocks();
    template<> inline FConcordFactorGraphBlocks& GetParameterBlocks<int32>() { return IntParameterBlocks; }
    template<> inline FConcordFactorGraphBlocks& GetParameterBlocks<float>() { return FloatParameterBlocks; }
    template<typename FValue> TArray<FValue>& GetParameterDefaultValues();
    template<> inline TArray<int32>& GetParameterDefaultValues<int32>() { return IntParameterDefaultValues; }
    template<> inline TArray<float>& GetParameterDefaultValues<float>() { return FloatParameterDefaultValues; }

    friend class FConcordCompiler;
    friend class UConcordNativeModel;
};
