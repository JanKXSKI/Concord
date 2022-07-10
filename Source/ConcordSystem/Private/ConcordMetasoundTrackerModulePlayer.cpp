// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "ConcordMetasoundTrackerModulePlayer.h"
#include "MetasoundLog.h"

using namespace Metasound;

TUniquePtr<IOperator> FConcordTrackerModulePlayerNode::FOperatorFactory::CreateOperator(const FBuildOperatorParams& InParams, 
                                                                                        FBuildResults& OutResults)
{
    const FInputVertexInterfaceData& Inputs = InParams.InputData;
    return MakeUnique<FConcordTrackerModulePlayerOperator>(InParams.OperatorSettings,
                                                           Inputs.GetOrCreateDefaultDataReadReference<FConcordMetasoundTrackerModuleAsset>("Tracker Module", InParams.OperatorSettings),
                                                           Inputs.GetOrCreateDefaultDataReadReference<FConcordMetasoundPatternAsset>("Pattern", InParams.OperatorSettings),
                                                           Inputs.GetOrCreateDefaultDataReadReference<FTrigger>("Start", InParams.OperatorSettings),
                                                           Inputs.GetOrCreateDefaultDataReadReference<FTrigger>("Stop", InParams.OperatorSettings),
                                                           Inputs.GetOrCreateDefaultDataReadReference<int32>("BPM", InParams.OperatorSettings),
                                                           Inputs.GetOrCreateDefaultDataReadReference<int32>("Lines per Beat", InParams.OperatorSettings),
                                                           Inputs.GetOrCreateDefaultDataReadReference<int32>("Start Line", InParams.OperatorSettings),
                                                           Inputs.GetOrCreateDefaultDataReadReference<int32>("Number of Lines", InParams.OperatorSettings),
                                                           Inputs.GetOrCreateDefaultDataReadReference<bool>("Loop", InParams.OperatorSettings));
}

const FVertexInterface& FConcordTrackerModulePlayerNode::DeclareVertexInterface()
{
    static const FVertexInterface VertexInterface(FInputVertexInterface(TInputDataVertex<FConcordMetasoundTrackerModuleAsset>("Tracker Module", { INVTEXT("The tracker module to play."), INVTEXT("Tracker Module") }),
                                                                        TInputDataVertex<FConcordMetasoundPatternAsset>("Pattern", { INVTEXT("The pattern to play."), INVTEXT("Pattern") }),
                                                                        TInputDataVertex<FTrigger>("Start", { INVTEXT("Start the Player."), INVTEXT("Start") }),
                                                                        TInputDataVertex<FTrigger>("Stop", { INVTEXT("Stop the Player."), INVTEXT("Stop") }),
                                                                        TInputDataVertex<int32>("BPM", { INVTEXT("The playback speed."), INVTEXT("BPM") }, 120),
                                                                        TInputDataVertex<int32>("Lines per Beat", { INVTEXT("The number of lines that make up a beat."), INVTEXT("Lines per Beat") }, 4),
                                                                        TInputDataVertex<int32>("Start Line", { INVTEXT("The line to start the Player at."), INVTEXT("Start Line") }, 0),
                                                                        TInputDataVertex<int32>("Number of Lines", { INVTEXT("The number of lines to play."), INVTEXT("Number of Lines") }, 32),
                                                                        TInputDataVertex<bool>("Loop", { INVTEXT("Loop the Player instead of stopping when finished."), INVTEXT("Loop") }, true)),
                                                  FOutputVertexInterface(TOutputDataVertex<FAudioBuffer>("Out Left", { INVTEXT("Left Audio Output"), INVTEXT("Out Left") }),
                                                                         TOutputDataVertex<FAudioBuffer>("Out Right", { INVTEXT("Right Audio Output"), INVTEXT("Out Right") })));
    return VertexInterface;
}

const FNodeClassMetadata& FConcordTrackerModulePlayerNode::GetNodeInfo()
{
    auto InitNodeInfo = []() -> FNodeClassMetadata
    {
        FNodeClassMetadata Info;
        Info.ClassName = { "Concord", "Tracker Module Player", "Default" };
        Info.MajorVersion = 1;
        Info.MinorVersion = 0;
        Info.DisplayName = INVTEXT("Concord Tracker Module Player");
        Info.Description = INVTEXT("Plays back an Impulse Tracker Module with Concord Pattern information.");
        Info.Author = TEXT("Jan Klimaschewski");
        Info.PromptIfMissing = INVTEXT("Missing :(");
        Info.DefaultInterface = DeclareVertexInterface();

        return Info;
    };

    static const FNodeClassMetadata Info = InitNodeInfo();

    return Info;
}

FConcordTrackerModulePlayerNode::FConcordTrackerModulePlayerNode(const FVertexName& InName, const FGuid& InInstanceID)
    :	FNode(InName, InInstanceID, GetNodeInfo())
    ,	Factory(MakeOperatorFactoryRef<FConcordTrackerModulePlayerNode::FOperatorFactory>())
    ,	Interface(DeclareVertexInterface())
{}
FConcordTrackerModulePlayerNode::FConcordTrackerModulePlayerNode(const FNodeInitData& InInitData)
    : FConcordTrackerModulePlayerNode(InInitData.InstanceName, InInitData.InstanceID)
{}

FDataReferenceCollection FConcordTrackerModulePlayerOperator::GetInputs() const
{
    FDataReferenceCollection InputDataReferences;
    InputDataReferences.AddDataReadReference("Tracker Module", TrackerModuleAsset);
    InputDataReferences.AddDataReadReference("Pattern", PatternAsset);
    InputDataReferences.AddDataReadReference("Start", Start);
    InputDataReferences.AddDataReadReference("Stop", Stop);
    InputDataReferences.AddDataReadReference("BPM", BPM);
    InputDataReferences.AddDataReadReference("Lines per Beat", LinesPerBeat);
    InputDataReferences.AddDataReadReference("Start Line", StartLine);
    InputDataReferences.AddDataReadReference("Number of Lines", NumberOfLines);
    InputDataReferences.AddDataReadReference("Loop", bLoop);
    return InputDataReferences;
}

FDataReferenceCollection FConcordTrackerModulePlayerOperator::GetOutputs() const
{
    FDataReferenceCollection OutputDataReferences;
    OutputDataReferences.AddDataReadReference("Out Left", LeftAudioOutput);
    OutputDataReferences.AddDataReadReference("Out Right", RightAudioOutput);
    return OutputDataReferences;
}

void FConcordTrackerModulePlayerOperator::Execute()
{
    if (!ReinitXmp()) return;
    if (!bCleared && PatternAsset->GetProxy()->Guid != CurrentPatternGuid && !PatternAsset->GetProxy()->ShouldChangePatternOnBeat())
        UpdatePattern();
    if (CurrentBPM != *BPM) UpdateBPM();
    if (CurrentLinesPerBeat != *LinesPerBeat) UpdateLinesPerBeat();

    if (Stop->IsTriggeredInBlock())
    {
        ClearPattern();
        bCleared = true;
    }
    else if (Start->IsTriggeredInBlock())
    {
        Start->ExecuteBlock([&](int32 BeginFrame, int32 EndFrame)
        {
            PlayModule(BeginFrame, EndFrame);
        },
        [&](int32 BeginFrame, int32 EndFrame)
        {
            if (bCleared) UpdatePattern();
            bCleared = false;
            SetPlayerStartPosition();
            PlayModule(BeginFrame, EndFrame);
        });
        return;
    }

    const int32 PrevRow = GetXmpRow();
    PlayModule(0, Settings.GetNumFramesPerBlock());
    const int32 CurrRow = GetXmpRow();
    if (PrevRow != CurrRow && (CurrRow + 1) % CurrentLinesPerBeat == 0 && !bCleared && PatternAsset->GetProxy()->Guid != CurrentPatternGuid)
        UpdatePattern();
}

bool FConcordTrackerModulePlayerOperator::ReinitXmp()
{
    if (!TrackerModuleAsset->IsInitialized() || !PatternAsset->IsInitialized())
        return false;

    if (!context)
    {
        context = xmp_create_context();
        return LoadTrackerModule();
    }
    else if (TrackerModuleAsset->GetProxy()->Guid != CurrentTrackerModuleGuid)
    {
        xmp_end_player(context);
        xmp_release_module(context);
        return LoadTrackerModule();
    }
    return true;
}

bool FConcordTrackerModulePlayerOperator::LoadTrackerModule()
{
    const FConcordTrackerModuleProxy* ModuleProxy = TrackerModuleAsset->GetProxy();
    if (int error_code = xmp_load_module_from_memory(context, ModuleProxy->ModuleDataPtr->GetData(), ModuleProxy->ModuleDataPtr->Num()))
    {
        UE_LOG(LogMetaSound, Error, TEXT("xmp_load_module_from_memory failed: %i"), -error_code);
        return false;
    }
    xmp_get_module_info(context, &module_info);
    if (int error_code = xmp_start_player(context, Settings.GetSampleRate(), 0))
    {
        UE_LOG(LogMetaSound, Error, TEXT("xmp_start_player failed: %i"), -error_code);
        return false;
    }
    xmp_set_player(context, XMP_PLAYER_MIX, 100);
    CurrentTrackerModuleGuid = ModuleProxy->Guid;
    if (module_info.mod->trk > 0)
        InitialNumberOfLines = module_info.mod->xxt[0]->rows;
    if (bCleared) ClearPattern();
    return true;
}

void FConcordTrackerModulePlayerOperator::FreeXmp()
{
    if (!ReinitXmp()) return;
    xmp_end_player(context);
    xmp_release_module(context);
    xmp_end_smix(context);
    xmp_free_context(context);
}

void FConcordTrackerModulePlayerOperator::SetPlayerStartPosition()
{
    if (int error_code = xmp_start_player(context, Settings.GetSampleRate(), 0))
    {
        UE_LOG(LogMetaSound, Error, TEXT("xmp_start_player failed: %i"), -error_code);
        return;
    }
    xmp_set_player(context, XMP_PLAYER_MIX, 100);
    const float RowDuration = (2.5f / FMath::Clamp(CurrentBPM, 32, 255)) * FMath::Max(1, 24 / CurrentLinesPerBeat); // https://wiki.openmpt.org/Manual:_Song_Properties#Tempo_Mode
    int32 FramesToSkip = *StartLine * RowDuration * Settings.GetSampleRate();
    while (FramesToSkip > 0)
    {
        int32 FramesSkipped = FMath::Min(FramesToSkip, Settings.GetNumFramesPerBlock());
        xmp_play_buffer(context, XMPBuffer.GetData(), FramesSkipped * 2 * sizeof(int16), 0);
        FramesToSkip -= FramesSkipped;
    }
}

void FConcordTrackerModulePlayerOperator::PlayModule(int32 StartFrame, int32 EndFrame)
{
    const int32 NumFrames = EndFrame - StartFrame;
    xmp_play_buffer(context, XMPBuffer.GetData(), NumFrames * 2 * sizeof(int16), *bLoop ? 0 : 1);
    for (int32 FrameIndex = 0; FrameIndex < NumFrames; ++FrameIndex)
    {
        LeftAudioOutput->GetData()[StartFrame + FrameIndex]  = XMPBuffer[FrameIndex * 2 + 0] / float(0x7FFF);
        RightAudioOutput->GetData()[StartFrame + FrameIndex] = XMPBuffer[FrameIndex * 2 + 1] / float(0x7FFF);
    }
    CheckNumberOfLines();
}

void FConcordTrackerModulePlayerOperator::UpdatePattern()
{
    ClearPattern();
    xmp_module* mod = module_info.mod;
    int32 track_index = 0;
    for (int32 instrument_index = 0; instrument_index < mod->ins; ++instrument_index)
    {
        TCHAR WideName[26];
        for (int32 Index = 0; Index < 26; ++Index)
            WideName[Index] = mod->xxi[instrument_index].name[Index];
        FStringView NameView = MakeStringView(WideName);
        const FConcordTrack* Track = PatternAsset->GetTracks().FindByHash(GetTypeHash(NameView), NameView);
        bool bIsRightChannel = false;
        if (!Track && (NameView[0] == 'M' || NameView[0] == 'L' || NameView[0] == 'R') && NameView[1] == '_')
        {
            NameView.RemovePrefix(2);
            Track = PatternAsset->GetTracks().FindByHash(GetTypeHash(NameView), NameView);
            bIsRightChannel = (WideName[0] == 'R');
        }
        if (!Track) continue;
        for (const FConcordColumn& Column : Track->Columns)
        {
            if (track_index >= mod->trk) return;
            xmp_track* track = mod->xxt[track_index++];
            for (int32 row = 0; row < track->rows; ++row)
            {
                int note = (Column.NoteValues.Num() > row) ? Column.NoteValues[row] : 0;
                if (note > 0) note = FMath::Min(note + 1, 128);
                else if (note < 0) note = XMP_KEY_OFF;

                int instrument = (Column.InstrumentValues.Num() > row) ? Column.InstrumentValues[row] : 0;
                if (instrument == 0 && note > 0) instrument = instrument_index + 1;
                else if (instrument != 0 && bIsRightChannel) ++instrument;
                if (note == XMP_KEY_OFF) instrument = 0;
                instrument = FMath::Clamp(instrument, 0, mod->ins);

                int vol = (Column.VolumeValues.Num() > row) ? Column.VolumeValues[row] : 0;
                vol = FMath::Clamp(vol, 0, 65);

                int delay = (Column.DelayValues.Num() > row) ? Column.DelayValues[row] : 0;
                delay = FMath::Clamp(delay, 0, 0x0F);

                xmp_event& event = track->event[row];
                event.note = note;
                event.ins = instrument;
                event.vol = vol;
                event.fxt = 0x0E;
                event.fxp = 0xD0 | delay;
            }
        }
    }
    CurrentPatternGuid = PatternAsset->GetProxy()->Guid;
    UpdateBPM();
    UpdateLinesPerBeat();
}

void FConcordTrackerModulePlayerOperator::UpdateBPM()
{
    CurrentBPM = *BPM;
    const int32 ClampedBPM = FMath::Clamp(CurrentBPM, 32, 255);
    if (ClampedBPM != CurrentBPM)
    {
        UE_LOG(LogMetaSound, Error, TEXT("Tracker Module playback requires 32 <= BPM <= 255. Clamping."));
    }
    if (module_info.mod->trk == 0) return;
    xmp_track* track = module_info.mod->xxt[0];
    for (int32 row = 0; row < track->rows; ++row)
    {
        xmp_event& event = track->event[row];
        event.f2t = 0x87;
        event.f2p = ClampedBPM;
    }
}

void FConcordTrackerModulePlayerOperator::UpdateLinesPerBeat()
{
    CurrentLinesPerBeat = *LinesPerBeat;
    const int32 Speed = FMath::Max(1, 24 / CurrentLinesPerBeat);
    if (24 % CurrentLinesPerBeat != 0)
    {
        UE_LOG(LogMetaSound, Error, TEXT("Tracker Module playback requires 24 % LinesPerBeat == 0. Truncating."));
    }
    if (module_info.mod->trk < 2)
    {
        UE_LOG(LogMetaSound, Error, TEXT("Tracker Module playback requires 2 tracks or more to dynamically set the LinesPerBeat."));
        return;
    }
    xmp_track* track = module_info.mod->xxt[1];
    for (int32 row = 0; row < track->rows; ++row)
    {
        xmp_event& event = track->event[row];
        event.f2t = 0x0f;
        event.f2p = Speed;
    }
}

int32 FConcordTrackerModulePlayerOperator::GetXmpRow() const
{
    xmp_frame_info frame_info;
    xmp_get_frame_info(context, &frame_info);
    return frame_info.row;
}

void FConcordTrackerModulePlayerOperator::CheckNumberOfLines()
{
    if (*NumberOfLines != CurrentNumberOfLines && *NumberOfLines > InitialNumberOfLines)
    {
        UE_LOG(LogMetaSound, Error, TEXT("Tracker Module playback does not support setting the number of lines above the initial number of lines in the module default pattern."));
        return;
    }
    CurrentNumberOfLines = *NumberOfLines;
    if (GetXmpRow() >= CurrentNumberOfLines)
    {
        if (*bLoop)
        {
            xmp_set_position(context, 0);
            UE_LOG(LogMetaSound, Warning, TEXT("Tracker Module playback is currently unprecise and includes clicks when looping with a number of lines not equal to the one in the default pattern."));
        }
        else
        {
            ClearPattern();
            bCleared = true;
        }
    }
}

void FConcordTrackerModulePlayerOperator::ClearPattern()
{
    xmp_module* mod = module_info.mod;
    for (int32 track_index = 0; track_index < mod->trk; ++track_index)
        for (int32 row = 0; row < mod->xxt[track_index]->rows; ++row)
        {
            xmp_event& event = mod->xxt[track_index]->event[row];
            event.note = XMP_KEY_OFF;
            event.fxt = 0; event.fxp = 0;
            event.f2t = 0; event.f2p = 0;
        }
}
