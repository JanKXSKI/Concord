// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordMarkovChainSampler.h"
#include "ConcordGibbsSampler.generated.h"

class CONCORDCORE_API FConcordGibbsSampler : public FConcordMarkovChainSampler
{
public:
    FConcordGibbsSampler(TSharedRef<const FConcordFactorGraph<float>> InFactorGraph,
                         TSharedRef<FConcordFactorGraphEnvironment<float>> InEnvironment,
                         bool bInMaximizeScore, int32 InBurnIn, int32 InMarginalsIterationCount);
private:
    TOptional<float> SamplingInitialization() override;
    void SamplingIteration() override;
    void SamplingIteration(float& InOutScore) override;

    TArray<float> Scores;
    TArray<double> Distribution;
};

UCLASS()
class CONCORDCORE_API UConcordGibbsSamplerFactory : public UConcordMarkovChainSamplerFactory
{
    GENERATED_BODY()
public:
    TSharedPtr<FConcordSampler> CreateSampler(TSharedRef<const FConcordFactorGraph<float>> FactorGraph, TOptional<FString>& OutErrorMessage) const override;
    EConcordCycleMode GetCycleMode() const override;
};
