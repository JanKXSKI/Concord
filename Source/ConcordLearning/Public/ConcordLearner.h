// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordCrate.h"
#include "Async/Async.h"

class CONCORDLEARNING_API FConcordLearner : public TSharedFromThis<FConcordLearner>
{
public:
    FConcordLearner(const TSharedRef<TArray<FConcordCrateData>>& InDataset);
    virtual ~FConcordLearner() {}
private:
    virtual double UpdateAndGetLoss() = 0;
public:
    virtual FConcordCrateData GetCrate() const = 0;
    virtual void OnConvergence() = 0;

    void Update(int32 InNumUpdatesToDo);
    TOptional<double> GetLossIfDoneUpdating();
    int32 GetNumUpdatesToDo() const { return NumUpdatesToDo; }
    int32 GetNumUpdatesCompleted() const { return NumUpdatesCompleted; }
    double GetPreviousLoss() const { return PreviousLoss; }
protected:
    TSharedRef<TArray<FConcordCrateData>> Dataset;
private:
    int32 NumUpdatesToDo;
    int32 NumUpdatesCompleted;
    double PreviousLoss;
    double CurrentLoss;
    TFuture<double> FutureLoss;
};
