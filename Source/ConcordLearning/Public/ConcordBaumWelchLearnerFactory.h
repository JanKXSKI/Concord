// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordLearnerFactory.h"
#include "ConcordBaumWelchLearner.h"
#include "FactorGraph/ConcordFactorGraph.h"
#include "ConcordBaumWelchLearnerFactory.generated.h"

UCLASS(DisplayName = "Baum-Welch Learner")
class CONCORDLEARNING_API UConcordBaumWelchLearnerFactory : public UConcordLearnerFactory
{
    GENERATED_BODY()
protected:
    bool CheckAndInit() override;
    TSharedPtr<FConcordLearner> CreateLearner() override;
public:
    UConcordBaumWelchLearnerFactory();

    UPROPERTY(EditAnywhere, Category = "Baum Welch")
    bool bSetDefaultCrates;

    UPROPERTY(EditAnywhere, Category = "Baum Welch", meta = (MinValue = 0))
    double ParameterInitDeviation;

    UPROPERTY(EditAnywhere, Category = "Baum Welch", meta = (MinValue = 0))
    double ParameterMutationDeviation;

    UPROPERTY(EditAnywhere, Category = "Baum Welch", meta = (MinValue = 0))
    double LearningRate;

    UPROPERTY(EditAnywhere, Category = "Baum Welch", meta = (MinValue = 0))
    float StabilityBias;
protected:
    TSharedRef<const FConcordFactorGraph<float>> GetFactorGraph() const;
    TUniquePtr<FConcordFactorGraphEnvironment<float>> MakeEnvironment() const;
    FConcordBaumWelchLearnerSettings MakeSettings() const;
private:
    TSharedPtr<const FConcordFactorGraph<float>> FactorGraph;
    FConcordBaumWelchNames Names;

    bool SetNames();
    bool CheckDataset() const;
};
