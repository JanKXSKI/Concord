// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordPattern.h"
#include "IAudioProxyInitializer.h"
#include "MetasoundDataTypeRegistrationMacro.h"

class FConcordPatternProxy;

namespace Metasound
{
    class CONCORDSYSTEM_API FConcordMetasoundPatternAsset
    {
    public:
        FConcordMetasoundPatternAsset() = default;
        FConcordMetasoundPatternAsset(const FConcordMetasoundPatternAsset&) = default;
        FConcordMetasoundPatternAsset& operator=(const FConcordMetasoundPatternAsset& Other) = default;
        FConcordMetasoundPatternAsset(const TUniquePtr<Audio::IProxyData>& InInitData);

        bool IsInitialized() const { return PatternProxy.IsValid(); }
        const TMap<FString, FConcordTrack>& GetTracks() const { return PatternProxy->GetTracks(); }
        const FConcordPatternProxy* GetProxy() const { return PatternProxy.Get(); }
    private:
        TSharedPtr<const FConcordPatternProxy> PatternProxy;
    };

    DECLARE_METASOUND_DATA_REFERENCE_TYPES(FConcordMetasoundPatternAsset, CONCORDSYSTEM_API, FConcordMetasoundPatternAssetTypeInfo, FConcordMetasoundPatternAssetReadRef, FConcordMetasoundPatternAssetWriteRef)
}
