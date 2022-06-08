// Copyright Jan Klimaschewski. All Rights Reserved.

#include "ConcordPattern.h"

TOptional<FConcordColumnPath> FConcordColumnPath::Parse(FStringView PathView)
{
    FConcordColumnPath Path;
    int32 DelimiterPosition;
    Path.ColumnIndex = 0;
    if (PathView.FindLastChar(TEXT(':'), DelimiterPosition))
    {
        LexFromString(Path.ColumnIndex, PathView.GetData() + DelimiterPosition + 1);
        if (Path.ColumnIndex < 0) return {};
        PathView.LeftInline(DelimiterPosition);
    }
    if (PathView.FindLastChar(TEXT('/'), DelimiterPosition))
    {
        if (DelimiterPosition + 1 == PathView.Len()) return {};
        switch (PathView[DelimiterPosition + 1])
        {
        case TEXT('n'):
        case TEXT('N'):
            Path.ColumnType = Note;
            break;
        case TEXT('i'):
        case TEXT('I'):
            Path.ColumnType = Instrument;
            break;
        case TEXT('v'):
        case TEXT('V'):
            Path.ColumnType = Volume;
            break;
        case TEXT('d'):
        case TEXT('D'):
            Path.ColumnType = Delay;
            break;
        default: return {};
        }
        PathView.LeftInline(DelimiterPosition);
    }
    else return {}; // require column type
    PathView.TrimStartAndEndInline();
    Path.TrackName = PathView;
    return Path;
}

void FConcordColumn::AddMidiNoop()
{
    NoteValues.Add(0);
    VolumeValues.Add(0);
    DelayValues.Add(0);
}

UConcordPattern::UConcordPattern()
#if WITH_EDITOR
    : PreviewBPM(120)
    , PreviewLinesPerBeat(4)
    , PreviewNumberOfLines(32)
    , PreviewStartSeconds(-1)
#endif
{}

#if WITH_EDITOR
void UConcordPattern::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
    Super::GetAssetRegistryTags(OutTags);

    if (AssetImportData)
        OutTags.Add(FAssetRegistryTag(SourceFileTagName(), AssetImportData->GetSourceData().ToJson(), FAssetRegistryTag::TT_Hidden));
}

FConcordCrateData UConcordPattern::GetCrate() const
{
    FConcordCrateData CrateData;
    auto& IntBlocks = CrateData.IntBlocks;
    for (const auto& NameTrackPair : PatternData.Tracks)
    {
        const auto& Track = NameTrackPair.Value;
        for (int32 ColumnIndex = 0; ColumnIndex < Track.Columns.Num(); ++ColumnIndex)
        {
            const auto& Column = Track.Columns[ColumnIndex];
            const FString Prefix = TEXT("In");
            const FString Suffix = (Track.Columns.Num() == 1) ? TEXT("") : FString::Printf(TEXT(":%i"), ColumnIndex);
            if (!Column.NoteValues.IsEmpty())
                IntBlocks.Add(FName(Prefix + NameTrackPair.Key + TEXT("/Notes") + Suffix)).Values = Column.NoteValues;
            if (!Column.InstrumentValues.IsEmpty())
                IntBlocks.Add(FName(Prefix + NameTrackPair.Key + TEXT("/Instruments") + Suffix)).Values = Column.InstrumentValues;
            if (!Column.VolumeValues.IsEmpty())
                IntBlocks.Add(FName(Prefix + NameTrackPair.Key + TEXT("/Volumes") + Suffix)).Values = Column.VolumeValues;
            if (!Column.DelayValues.IsEmpty())
                IntBlocks.Add(FName(Prefix + NameTrackPair.Key + TEXT("/Delays") + Suffix)).Values = Column.DelayValues;
        }
    }
    return MoveTemp(CrateData);
}

void UConcordPattern::StartPreview()
{
    PreviewStartSeconds = FPlatformTime::Seconds();
}

void UConcordPattern::StopPreview()
{
    PreviewStartSeconds = -1.0;
}

int32 UConcordPattern::GetPreviewLine(double Time) const
{
    if (PreviewStartSeconds < 0.0) return -1;
    const double Minutes = (Time - PreviewStartSeconds) / 60;
    const double Line = (PreviewBPM * Minutes) * PreviewLinesPerBeat + PreviewStartLine;
    return int32(Line) % PreviewNumberOfLines;
}
#endif

TUniquePtr<Audio::IProxyData> UConcordPattern::CreateNewProxyData(const Audio::FProxyDataInitParams& InitParams)
{
    if (!DataPtr) DataPtr = MakeShared<FConcordPatternData>();
    *DataPtr = PatternData;
    return MakeUnique<FConcordPatternProxy>(this);
}
