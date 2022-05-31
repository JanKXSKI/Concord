// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordFactorGraph.h"
#include "Async/TaskGraphInterfaces.h"

template<typename FFloatType>
class FConcordFactorGraphMaxSum
{
public:
    FConcordFactorGraphMaxSum(const FConcordFactorGraph<FFloatType>* InFactorGraph, const FConcordExpressionContextMutable<FFloatType>& InContext)
        : FactorGraph(InFactorGraph)
        , Context(InContext)
    {}

    void Init()
    {
        Messages.SetNum(FactorGraph->GetRandomVariableCount());
        for (int32 RootFlatRandomVariableIndex : FactorGraph->GetDisjointSubgraphRootFlatRandomVariableIndices())
            InitMessage(RootFlatRandomVariableIndex, nullptr);
    }

    FFloatType Run()
    {
        Reset();

        FGraphEventArray OutstandingEvents;
        for (int32 RootFlatRandomVariableIndex : FactorGraph->GetDisjointSubgraphRootFlatRandomVariableIndices())
            OutstandingEvents.Add(RunInward(RootFlatRandomVariableIndex, nullptr));
        FTaskGraphInterface::Get().WaitUntilTasksComplete(OutstandingEvents);

        FFloatType Score = 0;
        for (int32 RootFlatRandomVariableIndex : FactorGraph->GetDisjointSubgraphRootFlatRandomVariableIndices())
        {
            auto& Message = Messages[RootFlatRandomVariableIndex];
            if (Context.ObservationMask[RootFlatRandomVariableIndex])
            {
                Score += Message[Context.Variation[RootFlatRandomVariableIndex]].Score;
                RunOutward(RootFlatRandomVariableIndex, nullptr, Message[Context.Variation[RootFlatRandomVariableIndex]].NeighboringFactorOutwardValues);
            }
            else
            {
                TOptional<FFloatType> MaxRootScore;
                int32 RootValueWithMaxScore = 0;
                for (int32 RootValue = 0; RootValue < Message.Num(); ++RootValue)
                    if (!MaxRootScore || Message[RootValue].Score > MaxRootScore.GetValue())
                    {
                        MaxRootScore = Message[RootValue].Score;
                        RootValueWithMaxScore = RootValue;
                    }
                Context.Variation[RootFlatRandomVariableIndex] = RootValueWithMaxScore;
                Score += MaxRootScore.GetValue();
                RunOutward(RootFlatRandomVariableIndex, nullptr, Message[RootValueWithMaxScore].NeighboringFactorOutwardValues);
            }
        }
        return Score;
    }
private:
    const FConcordFactorGraph<FFloatType>* FactorGraph;
    FConcordExpressionContextMutable<FFloatType> Context;
    TArray<typename FConcordFactorHandleBase<FFloatType>::FMaxSumVariableMessage> Messages;
    friend class FMaxSumTask;

    void InitMessage(int32 FlatRandomVariableIndex, const FConcordFactorHandleBase<FFloatType>* ParentHandle)
    {
        const auto& NeighboringHandles = FactorGraph->GetNeighboringHandles(FlatRandomVariableIndex);
        Messages[FlatRandomVariableIndex].SetNum(FactorGraph->GetStateCount(FlatRandomVariableIndex));
        for (auto& MessageElement : Messages[FlatRandomVariableIndex])
        {
            MessageElement.Score = 0;
            MessageElement.NeighboringFactorOutwardValues.Reserve(NeighboringHandles.Num() - 1);
            for (const auto* NeighboringHandle : NeighboringHandles)
                if (NeighboringHandle != ParentHandle)
                    MessageElement.NeighboringFactorOutwardValues.FindOrAdd(NeighboringHandle).SetNumUninitialized(NeighboringHandle->GetNeighboringFlatRandomVariableIndices().Num() - 1);
        }
        for (const auto* NeighboringHandle : NeighboringHandles)
            if (NeighboringHandle != ParentHandle)
                for (int32 NeighboringFlatRandomVariableIndex : NeighboringHandle->GetNeighboringFlatRandomVariableIndices())
                    if (NeighboringFlatRandomVariableIndex != FlatRandomVariableIndex)
                        InitMessage(NeighboringFlatRandomVariableIndex, NeighboringHandle);
    }

    void Reset()
    {
        for (auto& Message : Messages) for (auto& Element : Message) Element.Score = 0;
    }

    class FMaxSumTask
    {
        const int32 FromIndex;
        const FConcordFactorHandleBase<FFloatType>* const ToHandle;
        FConcordFactorGraphMaxSum<FFloatType>* const MaxSum;
    public:
        FMaxSumTask(int32 InFromIndex, const FConcordFactorHandleBase<FFloatType>* InToHandle, FConcordFactorGraphMaxSum<FFloatType>* InMaxSum)
            : FromIndex(InFromIndex), ToHandle(InToHandle), MaxSum(InMaxSum)
        {}
        FORCEINLINE TStatId GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(FConcordExactSamplerMaxSumTask, STATGROUP_TaskGraphTasks); }
        static ENamedThreads::Type GetDesiredThread() { return ENamedThreads::AnyThread; }
        static ESubsequentsMode::Type GetSubsequentsMode() { return ESubsequentsMode::TrackSubsequents; }
        void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
        {
            for (const auto* NeighboringHandle : MaxSum->FactorGraph->GetNeighboringHandles(FromIndex))
                if (NeighboringHandle != ToHandle)
                    NeighboringHandle->AddMaxSumMessage(MaxSum->Context, MaxSum->Messages, FromIndex);
        }
    };

    FGraphEventRef RunInward(int32 FromIndex, const FConcordFactorHandleBase<FFloatType>* ToHandle)
    {
        FGraphEventArray OutstandingEvents;
        for (const auto* NeighboringHandle : FactorGraph->GetNeighboringHandles(FromIndex))
            if (NeighboringHandle != ToHandle)
                for (int32 NeighboringFlatRandomVariableIndex : NeighboringHandle->GetNeighboringFlatRandomVariableIndices())
                    if (NeighboringFlatRandomVariableIndex != FromIndex)
                        OutstandingEvents.Add(RunInward(NeighboringFlatRandomVariableIndex, NeighboringHandle));
        return TGraphTask<FMaxSumTask>::CreateTask(&OutstandingEvents).ConstructAndDispatchWhenReady(FromIndex, ToHandle, this);
    }

    void RunOutward(int32 FromIndex, const FConcordFactorHandleBase<FFloatType>* ParentHandle, const TMap<const FConcordFactorHandleBase<FFloatType>*, TArray<int32>>& NeighboringFactorOutwardValues)
    {
        for (const auto* NeighboringHandle : FactorGraph->GetNeighboringHandles(FromIndex))
            if (NeighboringHandle != ParentHandle)
            {
                int32 OutwardIndex = 0;
                for (int32 NeighboringFlatRandomVariableIndex : NeighboringHandle->GetNeighboringFlatRandomVariableIndices())
                    if (NeighboringFlatRandomVariableIndex != FromIndex)
                    {
                        const int32 Value = NeighboringFactorOutwardValues[NeighboringHandle][OutwardIndex++];
                        Context.Variation[NeighboringFlatRandomVariableIndex] = Value;
                        const auto& Message = Messages[NeighboringFlatRandomVariableIndex];
                        RunOutward(NeighboringFlatRandomVariableIndex, NeighboringHandle, Message[Value].NeighboringFactorOutwardValues);
                    }
            }
    }
};
