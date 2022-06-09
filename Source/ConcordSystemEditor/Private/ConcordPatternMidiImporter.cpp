// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "ConcordPatternMidiImporter.h"
#include "Widgets/SWindow.h"
#include "Interfaces/IMainFrameModule.h"
#include "Modules/ModuleManager.h"
#include "Application/SlateApplicationBase.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformApplicationMisc.h"
#include "PropertyEditorModule.h"
#include "Widgets/Input/SButton.h"

FConcordMidiParser::FConcordMidiParser(const FString& Filename, int32 InLinesPerBeat, int32 InTicksPerLine)
    : LinesPerBeat(InLinesPerBeat)
    , TicksPerLine(InTicksPerLine)
{
    MidiFile.read(TCHAR_TO_UTF8(*Filename));
    SetupTrackNames();
    MidiFile.linkNotePairs();
    MidiFile.joinTracks();
    TPQ = MidiFile.getTicksPerQuarterNote();
}

void FConcordMidiParser::SetupTrackNames()
{
    for (int32 TrackIndex = 0; TrackIndex < MidiFile.getNumTracks(); ++TrackIndex)
    {
        bool bHasNoteEvent = false, bAddedTrackName = false;
        for (int32 EventIndex = 0; EventIndex < MidiFile[TrackIndex].getSize(); ++EventIndex)
        {
            if (MidiFile[TrackIndex][EventIndex].isTrackName())
            {
                TrackNames.Add(TrackIndex, FString(MidiFile[TrackIndex][EventIndex].getMetaContent().c_str()));
                bAddedTrackName = true;
            }
            else if (MidiFile[TrackIndex][EventIndex].isNote())
            {
                bHasNoteEvent = true;
            }
            if (bAddedTrackName && bHasNoteEvent) break;
        }
        if (bHasNoteEvent && !bAddedTrackName)
            TrackNames.Add(TrackIndex, FString(TEXT("_unnamed_")) + LexToString(TrackIndex));
        else if (!bHasNoteEvent && bAddedTrackName)
            TrackNames.Remove(TrackIndex);
    }
}

FConcordMidiParser::FNoteEventIterator::FNoteEventIterator(const FConcordMidiParser& InParser, int32 InEventIndex)
    : Parser(InParser)
    , EventIndex(InEventIndex)
{}

FConcordMidiParser::FNoteEvent FConcordMidiParser::FNoteEventIterator::operator*() const
{
    return Parser.GetNoteEvent(EventIndex);
}

bool FConcordMidiParser::FNoteEventIterator::operator!=(const FNoteEventIterator& Other) const
{
    return EventIndex != Other.EventIndex || &Parser != &Other.Parser;
}

FConcordMidiParser::FNoteEventIterator& FConcordMidiParser::FNoteEventIterator::operator++()
{
    ++EventIndex;
    while (EventIndex < Parser.MidiFile[0].getSize())
    {
        if (Parser.MidiFile[0][EventIndex].isNoteOn()) return *this;
        ++EventIndex;
    }
    return *this;
}

FConcordMidiParser::FNoteEvents::FNoteEvents(const FConcordMidiParser& InParser)
    : Parser(InParser)
{}

FConcordMidiParser::FNoteEventIterator FConcordMidiParser::FNoteEvents::begin() const
{
    FNoteEventIterator Begin { Parser, -1 };
    return ++Begin;
}

FConcordMidiParser::FNoteEventIterator FConcordMidiParser::FNoteEvents::end() const
{
    return { Parser, Parser.MidiFile[0].getSize() };
}

FConcordMidiParser::FNoteEvents FConcordMidiParser::GetNoteEvents() const
{
    return FNoteEvents(*this);
}

FConcordMidiParser::FNoteEvent FConcordMidiParser::GetNoteEvent(int32 EventIndex) const
{
    const smf::MidiEvent& MidiEvent = MidiFile[0][EventIndex];
    check(MidiEvent.isNoteOn());
    FNoteEvent NoteEvent { TrackNames[MidiEvent.track] };
    NoteEvent.BeginLine = GetLine(MidiEvent.tick, NoteEvent.BeginDelay);
    NoteEvent.EndLine = GetLine(MidiEvent.getLinkedEvent()->tick, NoteEvent.EndDelay);
    if (NoteEvent.EndLine <= NoteEvent.BeginLine) { NoteEvent.EndLine = NoteEvent.BeginLine + 1; NoteEvent.EndDelay = 0; }
    NoteEvent.NoteValue = MidiEvent.getKeyNumber();
    NoteEvent.Velocity = MidiEvent.getVelocity();
    return NoteEvent;
}

int32 FConcordMidiParser::GetLine(int32 MidiTick, int32& OutDelay) const
{
    const float QuarterNotes = MidiTick / float(TPQ);
    const float FloatLine = QuarterNotes * LinesPerBeat;
    const int32 Line = FMath::TruncToInt(FloatLine);
    OutDelay = FMath::TruncToInt((FloatLine - Line) * TicksPerLine);
    return Line;
}

void UConcordPatternMidiImporter::ConfigureProperties()
{
    TSharedPtr<SWindow> ParentWindow;
    if(FModuleManager::Get().IsModuleLoaded( "MainFrame" ))
    {
        IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
        ParentWindow = MainFrame.GetParentWindow();
    }
    FVector2D WindowSize { 450.0f, 750.0f };
    FSlateRect WorkAreaRect = FSlateApplicationBase::Get().GetPreferredWorkArea();
    const FVector2D DisplayTopLeft(WorkAreaRect.Left, WorkAreaRect.Top);
    const FVector2D DisplaySize(WorkAreaRect.Right - WorkAreaRect.Left, WorkAreaRect.Bottom - WorkAreaRect.Top);
    const float ScaleFactor = FPlatformApplicationMisc::GetDPIScaleFactorAtPoint(DisplayTopLeft.X, DisplayTopLeft.Y);
    WindowSize *= ScaleFactor;
    const FVector2D WindowPosition = (DisplayTopLeft + (DisplaySize - WindowSize) / 2.0f) / ScaleFactor;

    TSharedRef<SWindow> Window = SNew(SWindow)
    .Title(INVTEXT("Import .mid as Concord Pattern"))
    .SizingRule(ESizingRule::Autosized)
    .AutoCenter(EAutoCenter::None)
    .ClientSize(WindowSize)
    .ScreenPosition(WindowPosition);

    FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    FDetailsViewArgs Args;
    Args.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Hide;
    Args.NameAreaSettings = FDetailsViewArgs::HideNameArea;
    TSharedRef<IDetailsView> FactoryDetailsView = PropertyModule.CreateDetailView(Args);
    FactoryDetailsView->HideFilterArea(true);
    FactoryDetailsView->SetObject(this);

    TSharedRef<SWidget> WindowContent = SNew(SVerticalBox)
    +SVerticalBox::Slot()
    .AutoHeight()
    [ FactoryDetailsView ]
    +SVerticalBox::Slot()
    .VAlign(VAlign_Bottom)
    .HAlign(HAlign_Center)
    .Padding(0.0f, 4.0f)
    [
        SNew(SButton)
        .VAlign(VAlign_Center)
        .Text(INVTEXT("Import"))
        .OnClicked_Lambda([Window]{ Window->RequestDestroyWindow(); return FReply::Handled(); })
    ];
    Window->SetContent(WindowContent);

    FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);
}

void UConcordPatternMidiImporter::Import(const FString& InFilename, UConcordPattern* InOutPattern) const
{
    FConcordMidiParser MidiParser(InFilename, LinesPerBeat, TicksPerLine);
    TMap<FString, FConcordTrack>& Tracks = InOutPattern->PatternData.Tracks;
    Tracks.Reset();
    MidiParser.InitTrackMap(Tracks);
    TMap<FString, TArray<int32>> CurrentlyActiveColumns;
    MidiParser.InitTrackMap(CurrentlyActiveColumns);
    int32 CurrentLine = 0;
    for (const FConcordMidiParser::FNoteEvent& NoteEvent : MidiParser.GetNoteEvents())
    {
        if (NoteEvent.BeginLine >= NumberOfLines) break;

        for (; CurrentLine <= NoteEvent.BeginLine; ++CurrentLine)
            for (auto& NameTrackPair : Tracks)
                for (int32 ColumnIndex = 0; ColumnIndex < NameTrackPair.Value.Columns.Num(); ++ColumnIndex)
                {
                    auto& Column = NameTrackPair.Value.Columns[ColumnIndex];
                    if (Column.NoteValues.Num() <= CurrentLine)
                        Column.AddMidiNoop();
                    else if (Column.NoteValues[CurrentLine] == CONCORD_PATTERN_NOTE_VALUE_OFF)
                        CurrentlyActiveColumns[NameTrackPair.Key].Remove(ColumnIndex);
                }

        const FString& TrackName = NoteEvent.TrackName;
        int32 ColumnIndex = 0;
        while (CurrentlyActiveColumns[TrackName].Contains(ColumnIndex)) ++ColumnIndex;
        TArray<FConcordColumn>& Columns = Tracks[TrackName].Columns;
        if (Columns.Num() <= ColumnIndex)
        {
            Columns.AddDefaulted();
            while (Columns[ColumnIndex].NoteValues.Num() <= NoteEvent.BeginLine) Columns[ColumnIndex].AddMidiNoop();
            Columns[ColumnIndex].NoteValues[0] = CONCORD_PATTERN_NOTE_VALUE_OFF;
        }
        check(Columns[ColumnIndex].NoteValues.Num() == NoteEvent.BeginLine + 1);
        Columns[ColumnIndex].NoteValues.Last() = NoteEvent.NoteValue;
        Columns[ColumnIndex].VolumeValues.Last() = NoteEvent.Velocity;
        Columns[ColumnIndex].DelayValues.Last() = NoteEvent.BeginDelay;
        CurrentlyActiveColumns[TrackName].Add(ColumnIndex);
        if (NoteEvent.EndLine >= NumberOfLines) continue;
        while (Columns[ColumnIndex].NoteValues.Num() < NoteEvent.EndLine) Columns[ColumnIndex].AddMidiNoop();
        Columns[ColumnIndex].NoteValues.Add(CONCORD_PATTERN_NOTE_VALUE_OFF);
        Columns[ColumnIndex].VolumeValues.Add(0);
        Columns[ColumnIndex].DelayValues.Add(NoteEvent.EndDelay);
    }
    for (; CurrentLine < NumberOfLines; ++CurrentLine)
        for (auto& NameTrackPair : Tracks)
            for (auto& Column : NameTrackPair.Value.Columns)
                if (Column.NoteValues.Num() <= CurrentLine)
                    Column.AddMidiNoop();
}
