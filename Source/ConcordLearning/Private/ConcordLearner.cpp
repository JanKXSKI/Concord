// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "ConcordLearner.h"

FConcordLearner::FConcordLearner(const TSharedRef<TArray<FConcordCrateData>>& InDataset)
    : Dataset(InDataset)
    , NumUpdatesToDo(0)
    , NumUpdatesCompleted(0)
    , PreviousLoss(TNumericLimits<double>::Max())
    , CurrentLoss(TNumericLimits<double>::Max())
{}

void FConcordLearner::Update(int32 InNumUpdatesToDo)
{
    NumUpdatesToDo = InNumUpdatesToDo;
    PreviousLoss = CurrentLoss;
    struct FUpdateCallable
    {
        FUpdateCallable(const TSharedRef<FConcordLearner>& InLearner) : Learner(InLearner) {}
        const TSharedRef<FConcordLearner> Learner;
        double operator()() const { return Learner->UpdateAndGetLoss(); }
    };
    FutureLoss = Async(EAsyncExecution::LargeThreadPool, FUpdateCallable(this->AsShared()));
}

TOptional<double> FConcordLearner::GetLossIfDoneUpdating()
{
    if (!FutureLoss.IsReady()) return {};
    CurrentLoss = FutureLoss.Get();
    FutureLoss.Reset();
    NumUpdatesCompleted += NumUpdatesToDo;
    return CurrentLoss;
}
