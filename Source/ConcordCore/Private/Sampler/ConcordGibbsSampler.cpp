// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "Sampler/ConcordGibbsSampler.h"

using namespace Concord;

FConcordGibbsSampler::FConcordGibbsSampler(TSharedRef<const FConcordFactorGraph<float>> InFactorGraph,
                                           TSharedRef<FConcordFactorGraphEnvironment<float>> InEnvironment,
                                           bool bInMaximizeScore, int32 InBurnIn, int32 InMarginalsIterationCount)
    : FConcordMarkovChainSampler(MoveTemp(InFactorGraph), MoveTemp(InEnvironment), bInMaximizeScore, InBurnIn, InMarginalsIterationCount)
{
    Scores.Reserve(32);
    Distribution.Reserve(32);
}

TOptional<float> FConcordGibbsSampler::SamplingInitialization()
{
    SamplingUtils.InitVariation();
    return {};
}

void FConcordGibbsSampler::SamplingIteration()
{
    for (int32 FlatRandomVariableIndex = 0; FlatRandomVariableIndex < GetFactorGraph()->GetRandomVariableCount(); ++FlatRandomVariableIndex)
    {
        if (GetEnvironment()->GetMask()[FlatRandomVariableIndex]) continue;
        SamplingUtils.ComputeConditionalDistribution(FlatRandomVariableIndex, Scores, Distribution);
        Variation[FlatRandomVariableIndex] = SampleDistribution(Distribution);
    }
}

void FConcordGibbsSampler::SamplingIteration(float& InOutScore)
{
    for (int32 FlatRandomVariableIndex = 0; FlatRandomVariableIndex < GetFactorGraph()->GetRandomVariableCount(); ++FlatRandomVariableIndex)
    {
        if (GetEnvironment()->GetMask()[FlatRandomVariableIndex]) continue;
        SamplingUtils.ComputeConditionalDistribution(FlatRandomVariableIndex, Scores, Distribution);
        const float PreviousScoreContribution = Scores[Variation[FlatRandomVariableIndex]];
        Variation[FlatRandomVariableIndex] = SampleDistribution(Distribution);
        InOutScore += Scores[Variation[FlatRandomVariableIndex]] - PreviousScoreContribution;
    }
}

TSharedPtr<FConcordSampler> UConcordGibbsSamplerFactory::CreateSampler(TSharedRef<const FConcordFactorGraph<float>> FactorGraph, TOptional<FString>& OutErrorMessage) const
{
    auto Environment = MakeShared<FConcordFactorGraphEnvironment<float>>(FactorGraph);
    TSharedRef<FConcordSampler> Sampler = MakeShared<FConcordGibbsSampler>(MoveTemp(FactorGraph), MoveTemp(Environment), bMaximizeScore, BurnIn, MarginalsIterationCount);
    return MoveTemp(Sampler);
}

EConcordCycleMode UConcordGibbsSamplerFactory::GetCycleMode() const
{
    return EConcordCycleMode::Ignore;
}
