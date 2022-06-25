// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordPattern.h"
#include "IAudioProxyInitializer.h"
#include "UObject/Object.h"
#include "EditorFramework/AssetImportData.h"
#include "Misc/Guid.h"
#include "ConcordTrackerModule.generated.h"

class CONCORDSYSTEM_API FConcordTrackerModuleProxy : public Audio::TProxyData<FConcordTrackerModuleProxy>
{
public:
    IMPL_AUDIOPROXY_CLASS(FConcordTrackerModuleProxy);

    explicit FConcordTrackerModuleProxy(TSharedPtr<TArray<uint8>> InModuleDataPtr)
        : Guid(FGuid::NewGuid())
        , ModuleDataPtr(InModuleDataPtr)
    {}
    FConcordTrackerModuleProxy(const FConcordTrackerModuleProxy& Other) = default;
    Audio::IProxyDataPtr Clone() const override { return MakeUnique<FConcordTrackerModuleProxy>(*this); }
    FGuid Guid;
    TSharedPtr<TArray<uint8>> ModuleDataPtr;
};

USTRUCT()
struct FConcordTrackerModuleInstrumentSlot
{
    GENERATED_BODY()

    FConcordTrackerModuleInstrumentSlot() : Index(0) {}
    FConcordTrackerModuleInstrumentSlot(int32 InIndex, FString InName) : Index(InIndex), Name(InName) {}

    UPROPERTY(VisibleAnywhere, Category = "Concord Tracker Module")
    int32 Index;

    UPROPERTY(VisibleAnywhere, Category = "Concord Tracker Module")
    FString Name;
};

UCLASS(BlueprintType, CollapseCategories)
class CONCORDSYSTEM_API UConcordTrackerModule : public UObject, public IAudioProxyDataFactory
{
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<uint8> ModuleData;

    UPROPERTY(VisibleAnywhere, Category = "Concord Tracker Module")
    TArray<FConcordTrackerModuleInstrumentSlot> InstrumentSlots;

    UPROPERTY(VisibleAnywhere, Category = "Concord Tracker Module")
    UConcordPattern* DefaultPattern;

#if WITH_EDITORONLY_DATA
    UPROPERTY(VisibleAnywhere, Instanced, Category = "ImportSettings")
    UAssetImportData* AssetImportData;

    void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override
    {
        Super::GetAssetRegistryTags(OutTags);

        if (AssetImportData)
            OutTags.Add(FAssetRegistryTag(SourceFileTagName(), AssetImportData->GetSourceData().ToJson(), FAssetRegistryTag::TT_Hidden));
    }
#endif

    TUniquePtr<Audio::IProxyData> CreateNewProxyData(const Audio::FProxyDataInitParams& InitParams) override
    {
        if (!ModuleDataPtr) ModuleDataPtr = MakeShared<TArray<uint8>>();
        *ModuleDataPtr = ModuleData;
        return MakeUnique<FConcordTrackerModuleProxy>(ModuleDataPtr);
    }
private:
    friend class FConcordTrackerModuleProxy;
    TSharedPtr<TArray<uint8>> ModuleDataPtr;
};
