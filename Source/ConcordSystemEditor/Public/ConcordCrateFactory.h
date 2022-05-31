// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "EditorReimportHandler.h"
#include "ConcordCrateFactory.generated.h"

UCLASS()
class CONCORDSYSTEMEDITOR_API UConcordCrateFactoryNew : public UFactory
{
    GENERATED_BODY()
public:
    UConcordCrateFactoryNew();
    UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

UCLASS()
class CONCORDSYSTEMEDITOR_API UConcordCrateFactory : public UFactory, public FReimportHandler
{
    GENERATED_BODY()
public:
    UConcordCrateFactory();

    UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;

    bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
    void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
    EReimportResult::Type Reimport(UObject* Obj) override;
private:
    void ImportCrateData(class UConcordCrate* Crate, const FString& Filename) const;
};
