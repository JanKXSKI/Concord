// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordLearner.h"
#include "FactorGraph/ConcordFactorGraph.h"
#include "FactorGraph/ConcordFactorGraphEnvironment.h"
#include "FactorGraph/ConcordFactorGraphSumProduct.h"

struct FConcordBaumWelchNames
{
    FName HiddenBox;
    FName Initial;
    FName Transition;
    struct FEmissionNames { FName ObservedBox, Emission; int32 Offset, Stride; };
    TArray<FEmissionNames> Emissions;
};

struct FConcordBaumWelchLearnerSettings
{
    TSharedRef<const FConcordFactorGraph<float>> FactorGraph;
    TUniquePtr<FConcordFactorGraphEnvironment<float>> Environment;
    FConcordBaumWelchNames Names;
    double InitDeviation;
    double MutationDeviation;
};

class CONCORDLEARNING_API FConcordBaumWelchLearner : public FConcordLearner
{
public:
    FConcordBaumWelchLearner(const TSharedRef<TArray<FConcordCrateData>>& InDataset, FConcordBaumWelchLearnerSettings InSettings);
    FConcordCrateData GetCrate() const override;
    void OnConvergence() override;
    void Mutate(double Deviation);
private:
    double UpdateAndGetLoss() override;
    FConcordExpressionContextMutable<float> GetExpressionContext() const;

    const FConcordFactorGraph<float>* GetFactorGraph() const { return &Settings.FactorGraph.Get(); }
    FConcordFactorGraphEnvironment<float>* GetEnvironment() const { return Settings.Environment.Get(); }
    const FConcordVariation& GetVariation() const { return Settings.Environment->GetStagingVariation(); }

    const FConcordFactorHandleBase<float>* GetNextHandle(int32 FlatRandomVariableIndex) const;
    void SetAlpha(int32 FlatRandomVariableIndex, const FConcordFactorHandleBase<float>* NextHandle);
    void SetBeta(int32 FlatRandomVariableIndex, int32 AlphaValue, const FConcordFactorHandleBase<float>* PreviousHandle, const TArrayView<float>& TransitionScores);
    static float ProbToScore(double Prob);

    FConcordBaumWelchLearnerSettings Settings;
    FConcordFactorGraphSumProduct<float, double, false> SumProduct;

    TArray<double> Alpha;
    TArray<double> AlphaMarg;
    TArray<double> Beta;

    TArray<double> InitialNumerators;
    TArray<double> TransitionNumerators;
    TArray<TArray<double>> EmissionNumerators;
    TArray<double> Denominators;
    TArray<double> EmissionDenominatorSummands;

    const FConcordFactorGraphBlock& HiddenBlock;
    const int32 HiddenStateCount;
};
