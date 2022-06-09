// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordPattern.h"
#include "MidiFile.h"
#include "ConcordPatternMidiImporter.generated.h"

struct CONCORDSYSTEMEDITOR_API FConcordMidiParser
{
    FConcordMidiParser(const FString& Filename, int32 InLinesPerBeat, int32 InTicksPerLine);

    struct CONCORDSYSTEMEDITOR_API FNoteEvent
    {
        const FString& TrackName;
        int32 BeginLine, BeginDelay, EndLine, EndDelay, NoteValue, Velocity;
    };

    struct CONCORDSYSTEMEDITOR_API FNoteEventIterator
    {
        FNoteEventIterator(const FConcordMidiParser& InParser, int32 InEventIndex);
        FNoteEvent operator*() const;
        bool operator!=(const FNoteEventIterator& Other) const;
        FNoteEventIterator& operator++();

        const FConcordMidiParser& Parser;
        int32 EventIndex;
    };

    struct CONCORDSYSTEMEDITOR_API FNoteEvents
    {
        FNoteEvents(const FConcordMidiParser& InParser);
        FNoteEventIterator begin() const;
        FNoteEventIterator end() const;

        const FConcordMidiParser& Parser;
    };

    template<typename FMapType>
    void InitTrackMap(FMapType& OutTracks) const
    {
        for (const auto& IndexNamePair : TrackNames)
            OutTracks.Add(IndexNamePair.Value);
    }
    FNoteEvents GetNoteEvents() const;
private:
    smf::MidiFile MidiFile;
    int32 TPQ;
    const int32 LinesPerBeat;
    const int32 TicksPerLine;

    TMap<int32, FString> TrackNames;
    void SetupTrackNames();

    friend FNoteEventIterator;
    friend FNoteEvents;
    FNoteEvent GetNoteEvent(int32 EventIndex) const;
    int32 GetLine(int32 MidiTick, int32& OutDelay) const;
};

UCLASS(CollapseCategories)
class CONCORDSYSTEMEDITOR_API UConcordPatternMidiImporter : public UConcordPatternImporter
{
    GENERATED_BODY()
public:
    UConcordPatternMidiImporter()
        : NumberOfLines(32)
        , LinesPerBeat(4)
        , TicksPerLine(6)
    {}

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Concord Midi Importer", meta = (ClampMin = 1))
    int32 NumberOfLines;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Concord Midi Importer", meta = (ClampMin = 1))
    int32 LinesPerBeat;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Concord Midi Importer", meta = (ClampMin = 1))
    int32 TicksPerLine;

    void ConfigureProperties();
    void Import(const FString& InFilename, UConcordPattern* InOutPattern) const override;

    UFUNCTION(BlueprintCallable, Category = "Concord Midi Importer")
    void Reimport(const FString& InFilename, UPARAM(ref) UConcordPattern*& InOutPattern, UConcordPattern*& OutPattern) { Import(InFilename, InOutPattern); OutPattern = InOutPattern; }
};
