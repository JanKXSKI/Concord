// Copyright Jan Klimaschewski. All Rights Reserved.

#include "ConcordCrateFactory.h"
#include "ConcordCrate.h"
#include "Misc/FileHelper.h"
#include "JsonObjectConverter.h"
#include "Editor.h"

UConcordCrateFactoryNew::UConcordCrateFactoryNew()
{
    SupportedClass = UConcordCrate::StaticClass();
    bCreateNew = true;
}

UObject* UConcordCrateFactoryNew::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
    return NewObject<UConcordCrate>(InParent, Class, Name, Flags, Context);
}

UConcordCrateFactory::UConcordCrateFactory()
{
    Formats.Add(TEXT("ccdc;JSON Crate File"));
    SupportedClass = UConcordCrate::StaticClass();
    bCreateNew = false;
    bEditorImport = true;
    SetAutomatedReimport(true);
}

UObject* UConcordCrateFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
    GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPreImport(this, InClass, InParent, InName, *FPaths::GetExtension(Filename));
    UConcordCrate* Crate = NewObject<UConcordCrate>(InParent, InName, Flags);
    Crate->AssetImportData = NewObject<UAssetImportData>(Crate, "AssetImportData");
    Crate->AssetImportData->Update(Filename);
    ImportCrateData(Crate, Filename);
    GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, Crate);
    return Crate;
}

bool UConcordCrateFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
    if (const UConcordCrate* Crate = Cast<UConcordCrate>(Obj))
    {
        if (!Crate->AssetImportData) return false;
        OutFilenames.Add(Crate->AssetImportData->GetFirstFilename());
        return true;
    }
    return false;
}

void UConcordCrateFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
    if (UConcordCrate* Crate = Cast<UConcordCrate>(Obj))
    {
        if (!Crate->AssetImportData) return;
        Crate->Modify();
        Crate->AssetImportData->Update(NewReimportPaths[0]);
    }
}

EReimportResult::Type UConcordCrateFactory::Reimport(UObject* Obj)
{
    UConcordCrate* Crate = Cast<UConcordCrate>(Obj);
    Crate->Modify();
    ImportCrateData(Crate, Crate->AssetImportData->GetFirstFilename());
    return EReimportResult::Succeeded;
}

void UConcordCrateFactory::ImportCrateData(UConcordCrate* Crate, const FString& Filename) const
{
    FString CrateString;
    if (FFileHelper::LoadFileToString(CrateString, *Filename))
        FJsonObjectConverter::JsonObjectStringToUStruct(CrateString, &Crate->CrateData);
}
