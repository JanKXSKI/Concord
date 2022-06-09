// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "ConcordPatternFactory.h"
#include "ConcordPattern.h"
#include "ConcordPatternMidiImporter.h"
#include "ConcordPatternJsonImporter.h"
#include "Editor.h"

UConcordPatternFactoryNew::UConcordPatternFactoryNew()
{
    SupportedClass = UConcordPattern::StaticClass();
    bCreateNew = true;
}

UObject* UConcordPatternFactoryNew::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
    return NewObject<UConcordPattern>(InParent, Class, Name, Flags, Context);
}

UConcordPatternFactory::UConcordPatternFactory()
{
    Formats.Add(TEXT("mid;Midi File"));
    Formats.Add(TEXT("ccdp;JSON Pattern File"));
    SupportedClass = UConcordPattern::StaticClass();
    bCreateNew = false;
    bEditorImport = true;
    SetAutomatedReimport(true);
}

bool UConcordPatternFactory::ConfigureProperties()
{
    if (ConfiguredMidiImporter) ConfiguredMidiImporter->MarkAsGarbage();
    ConfiguredMidiImporter = nullptr;
    return true;
}

UObject* UConcordPatternFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
    FString Extension = FPaths::GetExtension(Filename);
    GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPreImport(this, InClass, InParent, InName, *Extension);
    UConcordPattern* Pattern = NewObject<UConcordPattern>(InParent, InName, Flags);
    Pattern->AssetImportData = NewObject<UAssetImportData>(Pattern, "AssetImportData");
    Pattern->AssetImportData->Update(Filename);

    if (Extension == TEXT("mid"))
    {
        if (!ConfiguredMidiImporter)
        {
            ConfiguredMidiImporter = NewObject<UConcordPatternMidiImporter>();
            ConfiguredMidiImporter->ConfigureProperties();
        }
        Pattern->PatternImporter = DuplicateObject<UConcordPatternMidiImporter>(ConfiguredMidiImporter, Pattern);
    }
    else if (Extension == TEXT("pattern"))
    {
        Pattern->PatternImporter = NewObject<UConcordPatternImporter>(Pattern, UConcordPatternJsonImporter::StaticClass());
    }
    else checkNoEntry();
    Pattern->PatternImporter->Import(Filename, Pattern);

    GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, Pattern);
    return Pattern;
}

bool UConcordPatternFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
    if (const UConcordPattern* Pattern = Cast<UConcordPattern>(Obj))
    {
        if (!Pattern->AssetImportData) return false;
        OutFilenames.Add(Pattern->AssetImportData->GetFirstFilename());
        return true;
    }
    return false;
}

void UConcordPatternFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
    if (UConcordPattern* Pattern = Cast<UConcordPattern>(Obj))
    {
        if (!Pattern->AssetImportData) return;
        Pattern->Modify();
        Pattern->AssetImportData->Update(NewReimportPaths[0]);
    }
}

EReimportResult::Type UConcordPatternFactory::Reimport(UObject* Obj)
{
    UConcordPattern* Pattern = Cast<UConcordPattern>(Obj);
    Pattern->Modify();
    Pattern->PatternImporter->Import(Pattern->AssetImportData->GetFirstFilename(), Pattern);
    return EReimportResult::Succeeded;
}
