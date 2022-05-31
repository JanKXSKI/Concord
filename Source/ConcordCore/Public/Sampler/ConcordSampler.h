// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FactorGraph/ConcordFactorGraph.h"
#include "FactorGraph/ConcordFactorGraphEnvironment.h"
#include "FactorGraph/ConcordFactorGraphSamplingUtils.h"
#include "ConcordValue.h"
#include "ConcordPattern.h"
#include "Async/Async.h"
#include "ConcordSampler.generated.h"

class CONCORDCORE_API FConcordSampler : public TSharedFromThis<FConcordSampler>
{
public:
    FConcordSampler(TSharedRef<const FConcordFactorGraph<float>> InFactorGraph,
                    TSharedRef<FConcordFactorGraphEnvironment<float>> InEnvironment,
                    bool bInMaximizeScore);
    virtual ~FConcordSampler() {}
private:
    virtual float SampleVariation() = 0;
#if WITH_EDITOR
    virtual float SampleVariationAndInferMarginals(FConcordProbabilities& OutMarginals) = 0;
#endif
public:
    void GetVariationFromEnvironment();
    float SampleVariationSync();
    void SampleVariationAsync();
    bool IsSamplingVariation() const;
    TOptional<float> GetScoreIfDoneSampling();
#if WITH_EDITOR
    float SampleVariationAndInferMarginalsSync(FConcordProbabilities& OutMarginals);
#endif
    FConcordProbabilities GetConditionalProbabilities();

    void SetColumnsFromOutputs(FConcordPatternData& OutPatternData) const;
    void FillCrateWithOutputs(FConcordCrateData& OutCrateData) const;

    const FConcordVariation& GetVariation() const { return Variation; }
    FConcordExpressionContext<float> GetExpressionContext() const { return { Variation, GetEnvironment()->GetMask(), GetEnvironment()->GetIntParameters(), GetEnvironment()->GetFloatParameters() }; }
    FConcordExpressionContextMutable<float> GetExpressionContextMutable() { return { Variation, GetEnvironment()->GetMask(), GetEnvironment()->GetIntParameters(), GetEnvironment()->GetFloatParameters() }; }

    const FConcordFactorGraph<float>* GetFactorGraph() const { return &FactorGraph.Get(); }
    FConcordFactorGraphEnvironment<float>* GetEnvironment() const { return &Environment.Get(); }
protected:
    const bool bMaximizeScore;
    FConcordVariation Variation;
private:
    TSharedRef<const FConcordFactorGraph<float>> FactorGraph;
    TSharedRef<FConcordFactorGraphEnvironment<float>> Environment;
    TFuture<float> FutureScore;
protected:
    FConcordFactorGraphSamplingUtils<float> SamplingUtils;
private:
    void RunInstanceSamplers();
    void SetColumnFromOutput(const FConcordFactorGraph<float>::FOutput* Output, TArray<int32>& TargetArray, TArray<int32>* PreviousArray) const;
};

UCLASS(Abstract, EditInlineNew, CollapseCategories)
class CONCORDCORE_API UConcordSamplerFactory : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Concord Sampler")
    bool bMaximizeScore;

    virtual TSharedPtr<FConcordSampler> CreateSampler(TSharedRef<const FConcordFactorGraph<float>> FactorGraph, TOptional<FString>& OutErrorMessage) const PURE_VIRTUAL(UConcordSamplerFactory::CreateSampler, return {};)
    virtual EConcordCycleMode GetCycleMode() const PURE_VIRTUAL(UConcordSamplerFactory::GetCycleMode, return EConcordCycleMode::Error;)
};
