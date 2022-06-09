// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "ConcordLearnerFactory.h"
#include "DesktopPlatformModule.h"
#include "Framework/Application/SlateApplication.h"
#include "AssetToolsModule.h"
#include "HAL/FileManager.h"
#include "Misc/MessageDialog.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Misc/FileHelper.h"
#include "JsonObjectConverter.h"

DEFINE_LOG_CATEGORY(LogConcordLearning);

UConcordLearnerFactory::UConcordLearnerFactory()
    : DatasetDirectory(TEXT("./Dataset"))
    , OutputCrateAssetName(TEXT("LearnedParameters"))
    , NumConcurrentLearners(FGenericPlatformMisc::NumberOfCores())
    , NumUpdates(100)
    , LoggingInterval(10)
    , ConvergenceThreshold(1e-3)
    , bShouldStopLearning(false)
    , MinLoss(TNumericLimits<double>::Max())
    , NumUpdatesCompleted(0)
{}

void UConcordLearnerFactory::BrowseDatasetDirectory()
{
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (!DesktopPlatform) return;
    Modify();
    DesktopPlatform->OpenDirectoryDialog(FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr), TEXT("Browse Dataset Directory"), DatasetDirectory, DatasetDirectory);
}

void UConcordLearnerFactory::Learn()
{
    if (!Learners.IsEmpty())
    {
        FMessageDialog::Open(EAppMsgType::Ok, INVTEXT("Error: Already learning."));
        return;
    }

    if (!LoadDataset()) return;
    if (!CheckAndInit()) return;
    if (!SetupOutputCrate()) return;

    for (int32 LearnerIndex = 0; LearnerIndex < NumConcurrentLearners; ++LearnerIndex)
    {
        TSharedPtr<FConcordLearner> Learner = CreateLearner();
        if (!Learner)
        {
            CompleteLearning();
            return;
        }
        Learners.Add(Learner.ToSharedRef());
        Learner->Update(LoggingInterval);
    }

    FNotificationInfo Info(INVTEXT("Concord learning started."));
    Info.ExpireDuration = 3.0f;
    Info.bUseSuccessFailIcons = true;
    TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
    Notification->SetCompletionState(SNotificationItem::CS_Pending);

    const FText NotificationText = FText::Format(INVTEXT("Training {0}"), FText::FromString(GetModel()->GetName()));
    ProgressNotificationHandle = FSlateNotificationManager::Get().StartProgressNotification(NotificationText, NumConcurrentLearners * NumUpdates);
}

void UConcordLearnerFactory::StopLearning()
{
    bShouldStopLearning = true;
}

void UConcordLearnerFactory::Tick(float DeltaTime)
{
    if (bShouldStopLearning)
    {
        FSlateNotificationManager::Get().CancelProgressNotification(ProgressNotificationHandle);
        CompleteLearning();
        return;
    }

    bool bShouldUpdateProgressNotification = false;
    for (int32 LearnerIndex = 0; LearnerIndex < Learners.Num(); ++LearnerIndex)
    {
        TSharedRef<FConcordLearner>& Learner = Learners[LearnerIndex];
        TOptional<double> OptionalLoss = Learner->GetLossIfDoneUpdating();
        if (!OptionalLoss) continue;
        bShouldUpdateProgressNotification = true;

        double Loss = OptionalLoss.GetValue();
        UE_LOG(LogConcordLearning, Log, TEXT("Learner %2i Loss: %12.6f"), LearnerIndex, Loss);
        if (!FMath::IsFinite(Loss))
        {
            UE_LOG(LogConcordLearning, Log, TEXT("Learner %2i diverged, stopping."), LearnerIndex, Loss);
            FSlateNotificationManager::Get().CancelProgressNotification(ProgressNotificationHandle);
            CompleteLearning();
            return;
        }

        if (Loss < MinLoss)
        {
            MinLoss = Loss;
            MinLossParameterCrate = Learner->GetCrate();
        }

        NumUpdatesCompleted += Learner->GetNumUpdatesToDo();
        if (Learner->GetNumUpdatesCompleted() < NumUpdates)
        {
            if (FMath::Abs(Learner->GetPreviousLoss() - Loss) < ConvergenceThreshold)
            {
                UE_LOG(LogConcordLearning, Log, TEXT("Learner %2i converged."), LearnerIndex);
                Learner->OnConvergence();
            }
            Learner->Update(LoggingInterval);
        }
    }

    if (bShouldUpdateProgressNotification)
        FSlateNotificationManager::Get().UpdateProgressNotification(ProgressNotificationHandle, NumUpdatesCompleted, NumConcurrentLearners * NumUpdates);

    if (NumUpdatesCompleted >= NumConcurrentLearners * NumUpdates)
        CompleteLearning();
}

bool UConcordLearnerFactory::IsTickable() const
{
    return !Learners.IsEmpty();
}

TStatId UConcordLearnerFactory::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UConcordLearnerFactory, STATGROUP_Tickables);
}

UConcordModelBase* UConcordLearnerFactory::GetModel() const
{
    UConcordModelBase* Model = Cast<UConcordModelBase>(GetOuter());
    check(Model);
    return Model;
}

bool UConcordLearnerFactory::LoadDataset()
{
    Dataset = MakeShared<TArray<FConcordCrateData>>();

    FString FullDatasetDirectory;
    if (!FPaths::IsRelative(*DatasetDirectory)) FullDatasetDirectory = DatasetDirectory;
    else FullDatasetDirectory = FPackageName::LongPackageNameToFilename(FPaths::GetPath(GetPackage()->GetName())) / DatasetDirectory;
    FPaths::CollapseRelativeDirectories(FullDatasetDirectory);
    if (!IFileManager::Get().DirectoryExists(*FullDatasetDirectory))
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FString::Printf(TEXT("Error: Dataset directory %s does not exist."), *FullDatasetDirectory)));
        return false;
    }

    FString PackagePath;
    if (FPackageName::TryConvertFilenameToLongPackageName(FullDatasetDirectory, PackagePath))
    {
        TArray<FAssetData> AssetDatas;
        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        AssetRegistryModule.Get().GetAssetsByPath(FName(PackagePath), AssetDatas, true);
        for (const FAssetData& AssetData : AssetDatas)
            if (const UConcordCrate* Crate = Cast<UConcordCrate>(AssetData.GetAsset()))
                Dataset->Add(Crate->CrateData);
    }
    else
    {
        TArray<FString> DatasetFileNames;
        IFileManager::Get().FindFilesRecursive(DatasetFileNames, *FullDatasetDirectory, TEXT("*.ccdc"), true, false);
        FString FileContent;
        for (const FString& FileName : DatasetFileNames)
        {
            if (!FFileHelper::LoadFileToString(FileContent, *FileName)) continue;
            FConcordCrateData CrateData;
            if (!FJsonObjectConverter::JsonObjectStringToUStruct(FileContent, &CrateData)) continue;
            Dataset->Add(MoveTemp(CrateData));
        }
    }

    if (Dataset->IsEmpty())
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FString::Printf(TEXT("The dataset %s is empty. If the chosen directory is within an Unreal content folder, no ConcordCrate assets were found. Otherwise, no *.ccdc files were found."), *FullDatasetDirectory)));
        return false;
    }
    return true;
}

bool UConcordLearnerFactory::SetupOutputCrate()
{
    const FString OutputCratePackagePath = FPaths::GetPath(GetModel()->GetPackage()->GetName());
    const FString OutputCratePackageName = OutputCratePackagePath / OutputCrateAssetName;
    TArray<FAssetData> AssetDatas;
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    AssetRegistryModule.Get().GetAssetsByPackageName(FName(OutputCratePackageName), AssetDatas);
    if (!AssetDatas.IsEmpty())
        OutputCrate = Cast<UConcordCrate>(AssetDatas[0].GetAsset());
    if (!OutputCrate)
        OutputCrate = Cast<UConcordCrate>(FAssetToolsModule::GetModule().Get().CreateAsset(OutputCrateAssetName, OutputCratePackagePath, UConcordCrate::StaticClass(), nullptr));
    if (!OutputCrate)
    {
        FMessageDialog::Open(EAppMsgType::Ok, INVTEXT("Error: Output Crate Asset could not be located or created."));
        return false;
    }
    return true;
}

void UConcordLearnerFactory::CompleteLearning()
{
    Dataset.Reset();
    Learners.Reset();
    bShouldStopLearning = false;
    OutputCrate->Modify();
    OutputCrate->CrateData = MoveTemp(MinLossParameterCrate);
    MinLoss = TNumericLimits<double>::Max();
    MinLossParameterCrate = FConcordCrateData();
    NumUpdatesCompleted = 0;
    OutputCrate = nullptr;
    ProgressNotificationHandle = FProgressNotificationHandle();
    OnLearningCompleted();

    FNotificationInfo Info(INVTEXT("Concord learning complete."));
    Info.ExpireDuration = 3.0f;
    Info.bUseSuccessFailIcons = true;
    TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
    Notification->SetCompletionState(SNotificationItem::CS_Success);
}
