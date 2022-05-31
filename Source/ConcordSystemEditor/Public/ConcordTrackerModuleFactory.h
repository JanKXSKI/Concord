// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "EditorReimportHandler.h"
#include "ConcordTrackerModuleFactory.generated.h"

UCLASS()
class CONCORDSYSTEMEDITOR_API UConcordTrackerModuleFactory : public UFactory, public FReimportHandler
{
    GENERATED_BODY()
public:
    UConcordTrackerModuleFactory();

    UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;

    bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
    void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
    EReimportResult::Type Reimport(UObject* Obj) override;
private:
    void ImportTrackerModuleData(class UConcordTrackerModule* TrackerModule, const FString& Filename) const;
};
