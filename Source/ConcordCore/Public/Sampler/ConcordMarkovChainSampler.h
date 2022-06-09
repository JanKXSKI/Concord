// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordSampler.h"
#include "ConcordMarkovChainSampler.generated.h"

class CONCORDCORE_API FConcordMarkovChainSampler : public FConcordSampler
{
public:
    FConcordMarkovChainSampler(TSharedRef<const FConcordFactorGraph<float>> InFactorGraph,
                               TSharedRef<FConcordFactorGraphEnvironment<float>> InEnvironment,
                               bool bInMaximizeScore, int32 InBurnIn, int32 InMarginalsIterationCount);
private:
    float SampleVariation() override;
#if WITH_EDITOR
    float SampleVariationAndInferMarginals(FConcordProbabilities& OutMarginals) override;
#endif

    virtual TOptional<float> SamplingInitialization() = 0;
    virtual void SamplingIteration() = 0;
    virtual void SamplingIteration(float& InOutScore) = 0;

    const int32 BurnIn;
    const int32 MarginalsIterationCount;

    FConcordVariation MaxScoreVariation;
};

UCLASS(Abstract)
class CONCORDCORE_API UConcordMarkovChainSamplerFactory : public UConcordSamplerFactory
{
    GENERATED_BODY()
public:
    UConcordMarkovChainSamplerFactory()
        : BurnIn(200)
        , MarginalsIterationCount(500)
    {}

    UPROPERTY(EditAnywhere, Category = "Markov Chain Sampler", meta=(ClampMin=0))
    int32 BurnIn;

    UPROPERTY(EditAnywhere, Category = "Markov Chain Sampler", meta=(ClampMin=1))
    int32 MarginalsIterationCount;
};
