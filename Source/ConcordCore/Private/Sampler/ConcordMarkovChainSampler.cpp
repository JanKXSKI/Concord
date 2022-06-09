// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "Sampler/ConcordMarkovChainSampler.h"

FConcordMarkovChainSampler::FConcordMarkovChainSampler(TSharedRef<const FConcordFactorGraph<float>> InFactorGraph,
                                                       TSharedRef<FConcordFactorGraphEnvironment<float>> InEnvironment,
                                                       bool bInMaximizeScore, int32 InBurnIn, int32 InMarginalsIterationCount)
    : FConcordSampler(MoveTemp(InFactorGraph), MoveTemp(InEnvironment), bInMaximizeScore)
    , BurnIn(InBurnIn)
    , MarginalsIterationCount(InMarginalsIterationCount)
{
    if (bInMaximizeScore)
        MaxScoreVariation.SetNumUninitialized(GetFactorGraph()->GetRandomVariableCount());
}

float FConcordMarkovChainSampler::SampleVariation()
{
    TOptional<float> OptionalInitScore = SamplingInitialization();
    if (bMaximizeScore)
    {
        float Score = OptionalInitScore ? OptionalInitScore.GetValue() : SamplingUtils.GetScore();
        float MaxScore = Score;
        MaxScoreVariation = Variation;
        for (int32 IterationIndex = 0; IterationIndex < BurnIn + 1; ++IterationIndex)
        {
            SamplingIteration(Score);
            if (Score > MaxScore)
            {
                MaxScore = Score;
                MaxScoreVariation = Variation;
            }
        }
        Variation = MaxScoreVariation;
        return MaxScore;
    }
    else
    {
        for (int32 IterationIndex = 0; IterationIndex < BurnIn + 1; ++IterationIndex)
            SamplingIteration();
        return SamplingUtils.GetScore();
    }
}

#if WITH_EDITOR
float FConcordMarkovChainSampler::SampleVariationAndInferMarginals(FConcordProbabilities& OutMarginals)
{
    const float Score = SampleVariation();
    const TArray<int32> CachedVariation = Variation;

    TArray<TArray<int32>> Frequencies; Frequencies.SetNum(OutMarginals.Num());
    for (int32 FlatRandomVariableIndex = 0; FlatRandomVariableIndex < GetFactorGraph()->GetRandomVariableCount(); ++FlatRandomVariableIndex)
        Frequencies[FlatRandomVariableIndex].SetNumZeroed(GetFactorGraph()->GetStateCount(FlatRandomVariableIndex));

    for (int32 IterationIndex = 0; IterationIndex < MarginalsIterationCount; ++IterationIndex)
    {
        SamplingIteration();
        for (int32 FlatRandomVariableIndex = 0; FlatRandomVariableIndex < GetFactorGraph()->GetRandomVariableCount(); ++FlatRandomVariableIndex)
            ++Frequencies[FlatRandomVariableIndex][Variation[FlatRandomVariableIndex]];
    }
    for (int32 FlatRandomVariableIndex = 0; FlatRandomVariableIndex < GetFactorGraph()->GetRandomVariableCount(); ++FlatRandomVariableIndex)
    {
        OutMarginals[FlatRandomVariableIndex].SetNumUninitialized(Frequencies[FlatRandomVariableIndex].Num());
        for (int32 Value = 0; Value < Frequencies[FlatRandomVariableIndex].Num(); ++Value)
            OutMarginals[FlatRandomVariableIndex][Value] = Frequencies[FlatRandomVariableIndex][Value] / float(MarginalsIterationCount);
    }

    Variation = CachedVariation;
    return Score;
}
#endif
