// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "ConcordTrackerModuleFactory.h"
#include "ConcordTrackerModule.h"
#include "AssetToolsModule.h"
#include "Misc/FileHelper.h"
#include "Editor.h"
#include "xmp.h"

UConcordTrackerModuleFactory::UConcordTrackerModuleFactory()
{
    Formats.Add(TEXT("it;Impulse Tracker Module"));
    SupportedClass = UConcordTrackerModule::StaticClass();
    bCreateNew = false;
    bEditorImport = true;
    SetAutomatedReimport(true);
}

UObject* UConcordTrackerModuleFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
    GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPreImport(this, InClass, InParent, InName, *FPaths::GetExtension(Filename));
    UConcordTrackerModule* TrackerModule = NewObject<UConcordTrackerModule>(InParent, InName, Flags);
    TrackerModule->AssetImportData = NewObject<UAssetImportData>(TrackerModule, "AssetImportData");
    TrackerModule->AssetImportData->Update(Filename);

    FString PatternAssetName = InName.ToString() + TEXT("DefaultConcordPattern");
    FString PatternPackagePath = FPaths::GetPath(InParent->GetName());
    TrackerModule->DefaultPattern = Cast<UConcordPattern>(FAssetToolsModule::GetModule().Get().CreateAsset(PatternAssetName, PatternPackagePath, UConcordPattern::StaticClass(), nullptr));

    ImportTrackerModuleData(TrackerModule, Filename);
    GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, TrackerModule);
    return TrackerModule;
}

bool UConcordTrackerModuleFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
    if (const UConcordTrackerModule* TrackerModule = Cast<UConcordTrackerModule>(Obj))
    {
        if (!TrackerModule->AssetImportData) return false;
        OutFilenames.Add(TrackerModule->AssetImportData->GetFirstFilename());
        return true;
    }
    return false;
}

void UConcordTrackerModuleFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
    if (UConcordTrackerModule* TrackerModule = Cast<UConcordTrackerModule>(Obj))
    {
        if (!TrackerModule->AssetImportData) return;
        TrackerModule->Modify();
        TrackerModule->AssetImportData->Update(NewReimportPaths[0]);
    }
}

EReimportResult::Type UConcordTrackerModuleFactory::Reimport(UObject* Obj)
{
    UConcordTrackerModule* TrackerModule = Cast<UConcordTrackerModule>(Obj);
    TrackerModule->Modify();
    ImportTrackerModuleData(TrackerModule, TrackerModule->AssetImportData->GetFirstFilename());
    return EReimportResult::Succeeded;
}

int32 GetDelay(unsigned char fxt, unsigned char fxp)
{
    if (fxt != 0x0E) return 0;
    if ((fxp & 0xF0) != 0xD0) return 0;
    return fxp & 0x0F;
}

void UConcordTrackerModuleFactory::ImportTrackerModuleData(UConcordTrackerModule* TrackerModule, const FString& Filename) const
{
    TArray64<uint8> ModuleData;
    FFileHelper::LoadFileToArray(ModuleData, *Filename);
    TrackerModule->ModuleData = ModuleData;

    xmp_context context = xmp_create_context();
    if (xmp_load_module_from_memory(context, TrackerModule->ModuleData.GetData(), TrackerModule->ModuleData.Num()))
    {
        xmp_free_context(context);
        return;
    }
    xmp_module_info module_info;
    xmp_get_module_info(context, &module_info);
    const xmp_module* mod = module_info.mod;

    TrackerModule->InstrumentSlots.Reset();
    for (int32 instrument_index = 0; instrument_index < mod->ins; ++instrument_index)
        TrackerModule->InstrumentSlots.Add({ instrument_index + 1, mod->xxi[instrument_index].name });

    if (TrackerModule->DefaultPattern)
    {
        TrackerModule->DefaultPattern->Modify();
        FConcordPatternData& Pattern = TrackerModule->DefaultPattern->PatternData;
        Pattern.Tracks.Reset();
        for (int32 track_index = 0; track_index < mod->trk; ++track_index)
        {
            TOptional<FString> OptionalTrackName;
            FConcordColumn Column;
            Column.NoteValues.SetNumUninitialized(mod->xxt[track_index]->rows);
            Column.InstrumentValues.SetNumUninitialized(mod->xxt[track_index]->rows);
            Column.VolumeValues.SetNumUninitialized(mod->xxt[track_index]->rows);
            Column.DelayValues.SetNumUninitialized(mod->xxt[track_index]->rows);
            bool bHasInstrumentValues = false, bHasVolumeValues = false, bHasDelayValues = false;
            int32 TrackInstrument = 0;
            for (int32 row_index = 0; row_index < mod->xxt[track_index]->rows; ++row_index)
            {
                const xmp_event& event = mod->xxt[track_index]->event[row_index];
                if (event.ins > 0 && !OptionalTrackName)
                {
                    OptionalTrackName = mod->xxi[event.ins - 1].name;
                    TrackInstrument = event.ins;
                }

                Column.NoteValues[row_index] = (event.note >= XMP_KEY_OFF) ? -1 : (event.note > 0) ? event.note - 1 : 0;

                Column.InstrumentValues[row_index] = event.ins;
                bHasInstrumentValues |= (event.ins != TrackInstrument);

                Column.VolumeValues[row_index] = event.vol;
                bHasVolumeValues |= (event.vol > 0);

                Column.DelayValues[row_index] = FMath::Max(GetDelay(event.fxt, event.fxp), GetDelay(event.f2t, event.f2p));
                bHasDelayValues |= (Column.DelayValues[row_index] > 0);
            }
            if (!OptionalTrackName) continue;
            FString& Name = OptionalTrackName.GetValue();
            if (Name.Len() >= 2 && Name[1] == '_')
            {
                if (Name[0] == 'L' || Name[0] == 'M') Name.RightChopInline(2);
                else if (Name[0] == 'R') continue;
            }
            if (!bHasInstrumentValues) Column.InstrumentValues.Empty();
            if (!bHasVolumeValues) Column.VolumeValues.Empty();
            if (!bHasDelayValues) Column.DelayValues.Empty();
            FConcordTrack& Track = Pattern.Tracks.FindOrAdd(Name);
            Track.Columns.Add(MoveTemp(Column));
        }
    }

    xmp_release_module(context);
    xmp_free_context(context);
}
