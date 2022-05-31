// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordSampler.h"
#include "FactorGraph/ConcordFactorGraphMaxSum.h"
#include "FactorGraph/ConcordFactorGraphSumProduct.h"
#include "ConcordExactSampler.generated.h"

class CONCORDCORE_API FConcordExactSampler : public FConcordSampler
{
public:
    FConcordExactSampler(TSharedRef<const FConcordFactorGraph<float>> InFactorGraph,
                         TSharedRef<FConcordFactorGraphEnvironment<float>> InEnvironment,
                         bool bInMaximizeScore);
private:
    float SampleVariation() override;
#if WITH_EDITOR
    float SampleVariationAndInferMarginals(FConcordProbabilities& OutMarginals) override;
    bool bInitSumProductDone;
#endif
    FConcordFactorGraphMaxSum<float> MaxSum;
    FConcordFactorGraphSumProduct<float, double> SumProduct;

    float DoAncestralSampling();
    void DoAncestralSampling(const FConcordFactorHandleBase<float>* FromHandle, int32 ToIndex, TArray<double>& DistributionScratch);
    void AncestralSamplingImpl(const FConcordFactorHandleBase<float>* FromHandle, int32 ToIndex, int32 NeighboringIndicesIndex, int32 FactorMessageIndex, int32 FactorMessageStride, double& Acc);
#if WITH_EDITOR
    void GetMarginals(FConcordProbabilities& OutMarginals);
#endif
};

UCLASS()
class CONCORDCORE_API UConcordExactSamplerFactory : public UConcordSamplerFactory
{
    GENERATED_BODY()
public:
    UConcordExactSamplerFactory();
    
    UPROPERTY(EditAnywhere, Category = "Exact Sampler")
    bool bMergeCycles;

    UPROPERTY(EditAnywhere, Category = "Exact Sampler")
    uint64 ComplexityThreshold;

    TSharedPtr<FConcordSampler> CreateSampler(TSharedRef<const FConcordFactorGraph<float>> FactorGraph, TOptional<FString>& OutErrorMessage) const override;
    EConcordCycleMode GetCycleMode() const override;
private:
    uint64 GetComplexity(const FConcordFactorGraph<float>& FactorGraph) const;
};
