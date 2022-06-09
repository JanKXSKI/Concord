// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "EditorReimportHandler.h"
#include "ConcordPatternFactory.generated.h"

UCLASS()
class CONCORDSYSTEMEDITOR_API UConcordPatternFactoryNew : public UFactory
{
    GENERATED_BODY()
public:
    UConcordPatternFactoryNew();
    UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

UCLASS()
class CONCORDSYSTEMEDITOR_API UConcordPatternFactory : public UFactory, public FReimportHandler
{
    GENERATED_BODY()
public:
    UConcordPatternFactory();

    bool ConfigureProperties() override;
    UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;

    bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
    void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
    EReimportResult::Type Reimport(UObject* Obj) override;
private:
    UPROPERTY(Transient)
    class UConcordPatternMidiImporter* ConfiguredMidiImporter;
};
