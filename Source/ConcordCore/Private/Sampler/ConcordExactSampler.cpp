// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "Sampler/ConcordExactSampler.h"

using namespace Concord;

FConcordExactSampler::FConcordExactSampler(TSharedRef<const FConcordFactorGraph<float>> InFactorGraph,
                                           TSharedRef<FConcordFactorGraphEnvironment<float>> InEnvironment,
                                           bool bInMaximizeScore)
    : FConcordSampler(MoveTemp(InFactorGraph), MoveTemp(InEnvironment), bInMaximizeScore)
#if WITH_EDITOR
    , bInitSumProductDone(!bMaximizeScore)
#endif
    , MaxSum(GetFactorGraph(), GetExpressionContextMutable())
    , SumProduct(GetFactorGraph(), GetExpressionContextMutable())
{
    if (bMaximizeScore) MaxSum.Init();
    else SumProduct.Init();
}

float FConcordExactSampler::SampleVariation()
{
    if (bMaximizeScore) return MaxSum.Run();
    SumProduct.RunInward();
    return DoAncestralSampling();
}

#if WITH_EDITOR
float FConcordExactSampler::SampleVariationAndInferMarginals(FConcordProbabilities& OutMarginals)
{
    if (!bInitSumProductDone) SumProduct.Init();
    SumProduct.RunInward();
    const float Score = bMaximizeScore ? MaxSum.Run() : DoAncestralSampling();
    FConcordVariation VariationBackup = Variation;
    SumProduct.RunOutward();
    GetMarginals(OutMarginals);
    Variation = VariationBackup;
    return Score;
}

void FConcordExactSampler::GetMarginals(FConcordProbabilities& OutMarginals)
{
    TArray<double> DoubleDistribution;
    for (int32 FlatRandomVariableIndex = 0; FlatRandomVariableIndex < GetFactorGraph()->GetRandomVariableCount(); ++FlatRandomVariableIndex)
    {
        FConcordDistribution& Marginal = OutMarginals[FlatRandomVariableIndex];
        const int32 StateCount = GetFactorGraph()->GetStateCount(FlatRandomVariableIndex);
        if (GetEnvironment()->GetMask()[FlatRandomVariableIndex])
        {
            Marginal.Init(0.0f, StateCount);
            Marginal[Variation[FlatRandomVariableIndex]] = 1.0f;
        }
        else
        {
            DoubleDistribution.Init(1.0, StateCount);
            for (const FConcordFactorHandleBase<float>* Handle : GetFactorGraph()->GetNeighboringHandles(FlatRandomVariableIndex))
                for (int32 Value = 0; Value < StateCount; ++Value)
                    DoubleDistribution[Value] *= SumProduct.GetMessages().VariableMessageFactors[FlatRandomVariableIndex][Handle][Value];
            NormalizeDistribution(DoubleDistribution);
            Marginal.Empty(DoubleDistribution.Num());
            for (const double& Prob : DoubleDistribution)
                Marginal.Add(float(Prob));
        }
    }
}
#endif

float FConcordExactSampler::DoAncestralSampling()
{
    TArray<double> DoubleDistribution;
    for (int32 RootFlatRandomVariableIndex : GetFactorGraph()->GetDisjointSubgraphRootFlatRandomVariableIndices())
    {
        if (!GetEnvironment()->GetMask()[RootFlatRandomVariableIndex])
        {
            const int32 StateCount = GetFactorGraph()->GetStateCount(RootFlatRandomVariableIndex);
            DoubleDistribution.Init(1.0, StateCount);
            for (const FConcordFactorHandleBase<float>* NeighboringHandle : GetFactorGraph()->GetNeighboringHandles(RootFlatRandomVariableIndex))
                for (int32 Value = 0; Value < StateCount; ++Value)
                    DoubleDistribution[Value] *= SumProduct.GetMessages().VariableMessageFactors[RootFlatRandomVariableIndex][NeighboringHandle][Value];
            NormalizeDistribution(DoubleDistribution);
            Variation[RootFlatRandomVariableIndex] = SampleDistribution(DoubleDistribution);
        }
        for (const FConcordFactorHandleBase<float>* NeighboringHandle : GetFactorGraph()->GetNeighboringHandles(RootFlatRandomVariableIndex))
            for (int32 NeighboringFlatRandomVariableIndex : NeighboringHandle->GetNeighboringFlatRandomVariableIndices())
                if (NeighboringFlatRandomVariableIndex != RootFlatRandomVariableIndex)
                    DoAncestralSampling(NeighboringHandle, NeighboringFlatRandomVariableIndex, DoubleDistribution);
    }
    return SamplingUtils.GetScore();
}

void FConcordExactSampler::DoAncestralSampling(const FConcordFactorHandleBase<float>* FromHandle, int32 ToIndex, TArray<double>& DistributionScratch)
{
    if (!GetEnvironment()->GetMask()[ToIndex])
    {
        const int32 StateCount = GetFactorGraph()->GetStateCount(ToIndex);
        DistributionScratch.Init(0.0, StateCount);
        for (Variation[ToIndex] = 0; Variation[ToIndex] < StateCount; ++Variation[ToIndex])
            AncestralSamplingImpl(FromHandle, ToIndex, 0, 0, SumProduct.GetMessages().FactorMessages[FromHandle].Num(), DistributionScratch[Variation[ToIndex]]);
        NormalizeDistribution(DistributionScratch);
        Variation[ToIndex] = SampleDistribution(DistributionScratch);
    }

    for (const FConcordFactorHandleBase<float>* NeighboringHandle : GetFactorGraph()->GetNeighboringHandles(ToIndex))
        if (NeighboringHandle != FromHandle)
            for (int32 NeighboringFlatRandomVariableIndex : NeighboringHandle->GetNeighboringFlatRandomVariableIndices())
                if (NeighboringFlatRandomVariableIndex != ToIndex)
                    DoAncestralSampling(NeighboringHandle, NeighboringFlatRandomVariableIndex, DistributionScratch);
}

void FConcordExactSampler::AncestralSamplingImpl(const FConcordFactorHandleBase<float>* FromHandle, int32 ToIndex, int32 NeighboringIndicesIndex, int32 FactorMessageIndex, int32 FactorMessageStride, double& Acc)
{
    if (NeighboringIndicesIndex == FromHandle->GetNeighboringFlatRandomVariableIndices().Num())
    {
        Acc += SumProduct.GetMessages().FactorMessages[FromHandle][FactorMessageIndex];
        return;
    }

    const int32 FlatRandomVariableIndex = FromHandle->GetNeighboringFlatRandomVariableIndices()[NeighboringIndicesIndex];
    const int32 StateCount = GetFactorGraph()->GetStateCount(FlatRandomVariableIndex);
    FactorMessageStride /= StateCount;
    if (GetEnvironment()->GetMask()[FlatRandomVariableIndex] || Variation[FlatRandomVariableIndex] < StateCount) // inward pass leaves variation values at StateCount
    {
        FactorMessageIndex += Variation[FlatRandomVariableIndex] * FactorMessageStride;
        AncestralSamplingImpl(FromHandle, ToIndex, NeighboringIndicesIndex + 1, FactorMessageIndex, FactorMessageStride, Acc);
    }
    else for (Variation[FlatRandomVariableIndex] = 0; Variation[FlatRandomVariableIndex] < StateCount; ++Variation[FlatRandomVariableIndex])
    {
        AncestralSamplingImpl(FromHandle, ToIndex, NeighboringIndicesIndex + 1, FactorMessageIndex, FactorMessageStride, Acc);
        FactorMessageIndex += FactorMessageStride;
    }
}

UConcordExactSamplerFactory::UConcordExactSamplerFactory()
    : bMergeCycles(true)
    , ComplexityThreshold(1<<15)
{}

TSharedPtr<FConcordSampler> UConcordExactSamplerFactory::CreateSampler(TSharedRef<const FConcordFactorGraph<float>> FactorGraph, TOptional<FString>& OutErrorMessage) const
{
    if (FactorGraph->GetHasCycle())
    {
        OutErrorMessage = TEXT("Trying to create exact sampler for factor graph that contains a cycle.");
        return {};
    }
    const uint64 Complexity = FactorGraph->GetComplexity();
    if (Complexity > ComplexityThreshold)
    {
        OutErrorMessage = FString::Printf(TEXT("Complexity Threshold exceeded: %llu"), Complexity);
        return {};
    }
    auto Environment = MakeShared<FConcordFactorGraphEnvironment<float>>(FactorGraph);
    auto Sampler = MakeShared<FConcordExactSampler>(MoveTemp(FactorGraph), MoveTemp(Environment), bMaximizeScore);
    return MoveTemp(Sampler);
}

EConcordCycleMode UConcordExactSamplerFactory::GetCycleMode() const
{
    return bMergeCycles ? EConcordCycleMode::Merge : EConcordCycleMode::Error;
}
