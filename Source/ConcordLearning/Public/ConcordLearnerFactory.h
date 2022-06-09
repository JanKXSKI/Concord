// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordLearner.h"
#include "ConcordNativeModel.h"
#include "ConcordCrate.h"
#include "TickableEditorObject.h"
#include "Framework/Notifications/NotificationManager.h"
#include "ConcordLearnerFactory.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogConcordLearning, Log, All);

UCLASS(Abstract, EditInlineNew)
class CONCORDLEARNING_API UConcordLearnerFactory : public UConcordLearnerFactoryBase, public FTickableEditorObject
{
    GENERATED_BODY()
protected:
    virtual bool CheckAndInit() PURE_VIRTUAL(UConcordLearnerFactory::CheckAndInit, return false;)
    virtual TSharedPtr<FConcordLearner> CreateLearner() PURE_VIRTUAL(UConcordLearnerFactory::CreateLearner, return {};)
    virtual void OnLearningCompleted() {}
public:
    UConcordLearnerFactory();

    UPROPERTY(EditAnywhere, Category = "General")
    FString DatasetDirectory;

    UPROPERTY(EditAnywhere, Category = "General")
    FString OutputCrateAssetName;

    UPROPERTY(EditAnywhere, Category = "General", meta = (MinValue = 1))
    int32 NumConcurrentLearners;

    UPROPERTY(EditAnywhere, Category = "General", meta = (MinValue = 1))
    int32 NumUpdates;

    UPROPERTY(EditAnywhere, Category = "General", meta = (MinValue = 1))
    int32 LoggingInterval;

    UPROPERTY(EditAnywhere, Category = "General", meta = (MinValue = 0))
    double ConvergenceThreshold;

    void BrowseDatasetDirectory() override;
    void Learn() override;
    void StopLearning() override;

    void Tick(float DeltaTime) override;
    bool IsTickable() const override;
    TStatId GetStatId() const override;
protected:
    UConcordModelBase* GetModel() const;
    TSharedPtr<TArray<FConcordCrateData>> Dataset;
private:
    bool LoadDataset();
    bool SetupOutputCrate();
    void CompleteLearning();
    TArray<TSharedRef<FConcordLearner>> Learners;
    bool bShouldStopLearning;
    double MinLoss;
    FConcordCrateData MinLossParameterCrate;
    int32 NumUpdatesCompleted;

    UPROPERTY(Transient)
    UConcordCrate* OutputCrate;

    FProgressNotificationHandle ProgressNotificationHandle;
};
