// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordFactorGraph.h"

namespace Concord
{
    template<typename FFloatType>
    void NormalizeDistribution(TArray<FFloatType>& Distribution)
    {
        FFloatType Sum = 0; int32 InfCount = 0;
        for (FFloatType Prob : Distribution)
            if (!FMath::IsFinite(Prob)) ++InfCount;
            else Sum += Prob;
        if (InfCount > 0) for (FFloatType& Prob : Distribution) Prob = !FMath::IsFinite(Prob) ? 1.0f / InfCount : 0;
        else for (FFloatType& Prob : Distribution) Prob /= Sum;
    }

    template<typename FFloatType>
    int32 SampleDistribution(const TArray<FFloatType>& Distribution)
    {
        FFloatType Acc = 0.0f;
        const FFloatType Value = FMath::FRand();
        for (int32 Index = 0; Index < Distribution.Num(); ++Index)
        {
            Acc += Distribution[Index];
            if (Value < Acc) return Index;
        }
        return Distribution.Num() - 1;
    }

    template<typename FFloatType>
    void GetDistributionFromScores(const TArray<float>& Scores, TArray<FFloatType>& OutDistribution)
    {
        OutDistribution.SetNumUninitialized(Scores.Num(), false);
        for (int32 Value = 0; Value < OutDistribution.Num(); ++Value)
            OutDistribution[Value] = FMath::Exp(FFloatType(Scores[Value]));
        NormalizeDistribution(OutDistribution);
    }
}

template<typename FFloatType>
class FConcordFactorGraphSamplingUtils
{
public:
    FConcordFactorGraphSamplingUtils(const FConcordFactorGraph<FFloatType>* InFactorGraph, const FConcordExpressionContextMutable<FFloatType>& InContext)
        : FactorGraph(InFactorGraph)
        , Context(InContext)
    {}

    void InitVariation()
    {
        for (int32 FlatRandomVariableIndex = 0; FlatRandomVariableIndex < FactorGraph->GetRandomVariableCount(); ++FlatRandomVariableIndex)
        {
            if (Context.ObservationMask[FlatRandomVariableIndex]) continue;
            Context.Variation[FlatRandomVariableIndex] = FMath::RandRange(0, FactorGraph->GetStateCount(FlatRandomVariableIndex) - 1);
        }
    }

    void ComputeConditionalDistribution(int32 FlatRandomVariableIndex, TArray<float>& OutDistribution)
    {
        OutDistribution.Reset();
        OutDistribution.SetNumZeroed(FactorGraph->GetStateCount(FlatRandomVariableIndex), false);
        const int32 RememberedValue = Context.Variation[FlatRandomVariableIndex];

        if (Context.ObservationMask[FlatRandomVariableIndex])
        {
            OutDistribution[RememberedValue] = 1;
            return;
        }

        for (const auto& Handle : FactorGraph->GetNeighboringHandles(FlatRandomVariableIndex))
            Handle->AddScores(FlatRandomVariableIndex, Context, MakeArrayView(OutDistribution));
        Context.Variation[FlatRandomVariableIndex] = RememberedValue;
        Concord::GetDistributionFromScores(OutDistribution, OutDistribution);
    }

    template<typename FDistributionFloatType>
    void ComputeConditionalDistribution(int32 FlatRandomVariableIndex, TArray<float>& OutScores, TArray<FDistributionFloatType>& OutDistribution)
    {
        OutScores.Init(0, FactorGraph->GetStateCount(FlatRandomVariableIndex));
        const int32 RememberedValue = Context.Variation[FlatRandomVariableIndex];

        if (Context.ObservationMask[FlatRandomVariableIndex])
        {
            OutDistribution.Init(0, FactorGraph->GetStateCount(FlatRandomVariableIndex));
            OutDistribution[RememberedValue] = 1;
            return;
        }

        for (const auto& Handle : FactorGraph->GetNeighboringHandles(FlatRandomVariableIndex))
            Handle->AddScores(FlatRandomVariableIndex, Context, MakeArrayView(OutScores));
        Context.Variation[FlatRandomVariableIndex] = RememberedValue;
        Concord::GetDistributionFromScores(OutScores, OutDistribution);
    }

    FFloatType GetScore()
    {
        FFloatType Score = 0;
        for (const auto& Handle : FactorGraph->GetHandles())
            Score += Handle->ComputeScore(Context);
        return Score;
    }

    void GetIndexArray(TArray<int32>& OutArray)
    {
        OutArray.SetNumUninitialized(FactorGraph->GetRandomVariableCount());
        for (int32 FlatRandomVariableIndex = 0; FlatRandomVariableIndex < FactorGraph->GetRandomVariableCount(); ++FlatRandomVariableIndex)
            OutArray[FlatRandomVariableIndex] = FlatRandomVariableIndex;
    }
private:
    const FConcordFactorGraph<FFloatType>* FactorGraph;
    FConcordExpressionContextMutable<FFloatType> Context;
};
