// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IAudioProxyInitializer.h"
#include "UObject/Object.h"
#include "EditorFramework/AssetImportData.h"
#include "Misc/Guid.h"
#include "ConcordCrate.h"
#include "ConcordPattern.generated.h"

#define CONCORD_PATTERN_NOTE_VALUE_OFF (-1)

struct CONCORDCORE_API FConcordColumnPath
{
    static TOptional<FConcordColumnPath> Parse(FStringView PathView);
    FStringView TrackName;
    enum EColumnType { Note, Instrument, Volume, Delay } ColumnType;
    int32 ColumnIndex;
};

USTRUCT(BlueprintType)
struct CONCORDCORE_API FConcordColumn
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Concord Pattern")
    TArray<int32> NoteValues;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Concord Pattern")
    TArray<int32> InstrumentValues;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Concord Pattern")
    TArray<int32> VolumeValues;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Concord Pattern")
    TArray<int32> DelayValues;

    void AddMidiNoop();
};

USTRUCT(BlueprintType)
struct FConcordTrack
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Concord Pattern")
    TArray<FConcordColumn> Columns;
};

USTRUCT(BlueprintType)
struct CONCORDCORE_API FConcordPatternData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Concord Pattern")
    TMap<FString, FConcordTrack> Tracks;
};

UCLASS(Abstract, BlueprintType)
class CONCORDCORE_API UConcordPatternImporter : public UObject
{
    GENERATED_BODY()
public:
    virtual void Import(const FString& InFilename, UConcordPattern* InOutPattern) const PURE_VIRTUAL(UConcordPatternImporter::Import)
};

UCLASS(BlueprintType)
class CONCORDCORE_API UConcordPattern : public UObject, public IAudioProxyDataFactory
{
    GENERATED_BODY()
public:
    UConcordPattern();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Concord Pattern")
    FConcordPatternData PatternData;

#if WITH_EDITORONLY_DATA
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Instanced, Category = "ImportSettings")
    class UAssetImportData* AssetImportData;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, NoClear, Category = "Concord Pattern")
    class UConcordPatternImporter* PatternImporter;

    UPROPERTY(EditAnywhere, Category = "Preview")
    class USoundBase* PreviewSound;

    UPROPERTY(EditAnywhere, Category = "Preview", meta = (ClampMin = 0))
    int32 PreviewStartLine;

    UPROPERTY(EditAnywhere, Category = "Preview", meta = (ClampMin = 1))
    int32 PreviewBPM;

    UPROPERTY(EditAnywhere, Category = "Preview", meta = (ClampMin = 1))
    int32 PreviewLinesPerBeat;

    UPROPERTY(EditAnywhere, Category = "Preview", meta = (ClampMin = 1))
    int32 PreviewNumberOfLines;
#endif
#if WITH_EDITOR
    void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;

    UFUNCTION(BlueprintPure, Category = "Concord Pattern")
    FConcordCrateData GetCrate() const;

    double PreviewStartSeconds;
    void StartPreview();
    void StopPreview();
    int32 GetPreviewLine(double Time) const;
#endif
    TUniquePtr<Audio::IProxyData> CreateNewProxyData(const Audio::FProxyDataInitParams& InitParams) override;
private:
    friend class FConcordPatternProxy;
    TSharedPtr<FConcordPatternData> DataPtr;
};

class CONCORDCORE_API FConcordPatternProxy : public Audio::TProxyData<FConcordPatternProxy>
{
public:
    IMPL_AUDIOPROXY_CLASS(FConcordPatternProxy);

    explicit FConcordPatternProxy(UConcordPattern* InPattern) : Guid(FGuid::NewGuid()), DataPtr(InPattern->DataPtr) {}
    FConcordPatternProxy(const FConcordPatternProxy& Other) = default;
    Audio::IProxyDataPtr Clone() const override { return MakeUnique<FConcordPatternProxy>(*this); }

    const TMap<FString, FConcordTrack>& GetTracks() const { return DataPtr->Tracks; }
    FGuid Guid;
private:
    TSharedPtr<FConcordPatternData> DataPtr;
};
