// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordFactorGraph.h"
#include "Async/TaskGraphInterfaces.h"
#include <type_traits>

template<typename FFloatType, typename FSumProductMessageFloatType = FFloatType, bool bParallelize = true>
class FConcordFactorGraphSumProduct
{
public:
    FConcordFactorGraphSumProduct(const FConcordFactorGraph<FFloatType>* InFactorGraph, const FConcordExpressionContextMutable<FFloatType>& InContext)
        : FactorGraph(InFactorGraph)
        , Context(InContext)
    {}

    void Init()
    {
        Messages.FactorMessages.Reserve(FactorGraph->GetHandles().Num());
        for (const auto& Handle : FactorGraph->GetHandles())
        {
            TArray<FSumProductMessageFloatType>& Values = Messages.FactorMessages.FindOrAdd(Handle.Get()); int32 NumValues = 1;
            for (int32 NeighboringFlatRandomVariableIndex : Handle->GetNeighboringFlatRandomVariableIndices())
                NumValues *= FactorGraph->GetStateCount(NeighboringFlatRandomVariableIndex);
            Values.Init(1, NumValues);
        }
        Messages.VariableMessageFactors.SetNum(FactorGraph->GetRandomVariableCount());
        for (int32 FlatRandomVariableIndex = 0; FlatRandomVariableIndex < FactorGraph->GetRandomVariableCount(); ++FlatRandomVariableIndex)
        {
            Messages.VariableMessageFactors[FlatRandomVariableIndex].Reserve(FactorGraph->GetNeighboringHandles(FlatRandomVariableIndex).Num());
            for (const auto* NeighboringHandle : FactorGraph->GetNeighboringHandles(FlatRandomVariableIndex))
                Messages.VariableMessageFactors[FlatRandomVariableIndex].FindOrAdd(NeighboringHandle).Init(0, FactorGraph->GetStateCount(FlatRandomVariableIndex));
        }
    }

    void RunInward()
    {
        Reset();
        if constexpr (bParallelize)
        {
            FGraphEventArray OutstandingEvents;
            for (int32 RootFlatRandomVariableIndex : FactorGraph->GetDisjointSubgraphRootFlatRandomVariableIndices())
                OutstandingEvents.Add(RunInwardParallelized(RootFlatRandomVariableIndex, nullptr));
            FTaskGraphInterface::Get().WaitUntilTasksComplete(OutstandingEvents);
        }
        else
        {
            for (int32 RootFlatRandomVariableIndex : FactorGraph->GetDisjointSubgraphRootFlatRandomVariableIndices())
                RunInward(RootFlatRandomVariableIndex, nullptr);
        }
    }

    void RunOutward()
    {
        for (int32 RootFlatRandomVariableIndex : FactorGraph->GetDisjointSubgraphRootFlatRandomVariableIndices())
            RunOutward(RootFlatRandomVariableIndex, nullptr);
    }

    const auto& GetMessages() const { return Messages; }

    // Call after RunInward()
    FSumProductMessageFloatType GetZ() const
    {
        FSumProductMessageFloatType Z = 1;
        for (int32 RootFlatRandomVariableIndex : FactorGraph->GetDisjointSubgraphRootFlatRandomVariableIndices())
        {
            FSumProductMessageFloatType DisjointSubgraphZ = 0;
            for (int32 Value = 0; Value < FactorGraph->GetStateCount(RootFlatRandomVariableIndex); ++Value)
            {
                FSumProductMessageFloatType UnnormalizedProb = 1;
                for (const FConcordFactorHandleBase<FFloatType>* NeighboringHandle : FactorGraph->GetNeighboringHandles(RootFlatRandomVariableIndex))
                    UnnormalizedProb *= Messages.VariableMessageFactors[RootFlatRandomVariableIndex][NeighboringHandle][Value];
                DisjointSubgraphZ += UnnormalizedProb;
            }
            Z *= DisjointSubgraphZ;
        }
        return Z;
    }

    const auto& GetVariableMessageFactors() const { return Messages.VariableMessageFactors; }
private:
    const FConcordFactorGraph<FFloatType>* FactorGraph;
    FConcordExpressionContextMutable<FFloatType> Context;
    typename FConcordFactorHandleBase<FFloatType>:: template FSumProductMessages<FSumProductMessageFloatType> Messages;
    friend class FSumProductTask;

    void Reset()
    {
        for (auto& HandleValuesPair : Messages.FactorMessages) for (auto& Value : HandleValuesPair.Value) Value = 1;
        for (auto& VariableMessageFactors : Messages.VariableMessageFactors) for (auto& HandleValuesPair : VariableMessageFactors) for (auto& Value : HandleValuesPair.Value) Value = 0;
    }

    void SendSumProductMessage(const FConcordFactorHandleBase<FFloatType>* FromHandle, int32 TargetFlatRandomVariableIndex)
    {
        if constexpr (std::is_same_v<FSumProductMessageFloatType, double>)
            FromHandle->SendSumProductMessageDouble(Context, Messages, TargetFlatRandomVariableIndex);
        else
            FromHandle->SendSumProductMessage(Context, Messages, TargetFlatRandomVariableIndex);
    }

    class FSumProductTask
    {
        const int32 FromIndex;
        const FConcordFactorHandleBase<FFloatType>* const ToHandle;
        FConcordFactorGraphSumProduct<FFloatType, FSumProductMessageFloatType>* const SumProduct;
    public:
        FSumProductTask(int32 InFromIndex, const FConcordFactorHandleBase<FFloatType>* InToHandle, FConcordFactorGraphSumProduct<FFloatType, FSumProductMessageFloatType>* InSumProduct)
            : FromIndex(InFromIndex), ToHandle(InToHandle), SumProduct(InSumProduct)
        {}
        FORCEINLINE TStatId GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(FConcordExactSamplerSumProductTask, STATGROUP_TaskGraphTasks); }
        static ENamedThreads::Type GetDesiredThread() { return ENamedThreads::AnyThread; }
        static ESubsequentsMode::Type GetSubsequentsMode() { return ESubsequentsMode::TrackSubsequents; }
        void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
        {
            for (const FConcordFactorHandleBase<FFloatType>* NeighboringHandle : SumProduct->FactorGraph->GetNeighboringHandles(FromIndex))
                if (NeighboringHandle != ToHandle)
                    SumProduct->SendSumProductMessage(NeighboringHandle, FromIndex);
        }
    };

    FGraphEventRef RunInwardParallelized(int32 FromIndex, const FConcordFactorHandleBase<FFloatType>* ToHandle)
    {
        FGraphEventArray OutstandingEvents;
        for (const auto* NeighboringHandle : FactorGraph->GetNeighboringHandles(FromIndex))
            if (NeighboringHandle != ToHandle)
                for (int32 NeighboringFlatRandomVariableIndex : NeighboringHandle->GetNeighboringFlatRandomVariableIndices())
                    if (NeighboringFlatRandomVariableIndex != FromIndex)
                        if (FactorGraph->GetNeighboringHandles(NeighboringFlatRandomVariableIndex).Num() > 1) // only spawn a task if there is work to do
                            OutstandingEvents.Add(RunInwardParallelized(NeighboringFlatRandomVariableIndex, NeighboringHandle));
        return TGraphTask<FSumProductTask>::CreateTask(&OutstandingEvents).ConstructAndDispatchWhenReady(FromIndex, ToHandle, this);
    }

    void RunInward(int32 FromIndex, const FConcordFactorHandleBase<FFloatType>* ToHandle)
    {
        for (const auto* NeighboringHandle : FactorGraph->GetNeighboringHandles(FromIndex))
            if (NeighboringHandle != ToHandle)
                for (int32 NeighboringFlatRandomVariableIndex : NeighboringHandle->GetNeighboringFlatRandomVariableIndices())
                    if (NeighboringFlatRandomVariableIndex != FromIndex)
                        RunInward(NeighboringFlatRandomVariableIndex, NeighboringHandle);

        for (const auto* NeighboringHandle : FactorGraph->GetNeighboringHandles(FromIndex))
            if (NeighboringHandle != ToHandle)
                SendSumProductMessage(NeighboringHandle, FromIndex);
    }

    void RunOutward(int32 FromIndex, const FConcordFactorHandleBase<float>* ParentHandle)
    {
        for (const auto* NeighboringHandle : FactorGraph->GetNeighboringHandles(FromIndex))
            if (NeighboringHandle != ParentHandle)
                for (int32 NeighboringFlatRandomVariableIndex : NeighboringHandle->GetNeighboringFlatRandomVariableIndices())
                    if (NeighboringFlatRandomVariableIndex != FromIndex)
                    {
                        SendSumProductMessage(NeighboringHandle, NeighboringFlatRandomVariableIndex);
                        RunOutward(NeighboringFlatRandomVariableIndex, NeighboringHandle);
                    }
    }
};
