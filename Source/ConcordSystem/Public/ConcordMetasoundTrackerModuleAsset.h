// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTrackerModule.h"
#include "IAudioProxyInitializer.h"
#include "MetasoundDataTypeRegistrationMacro.h"

class FConcordTrackerModuleProxy;

namespace Metasound
{
    class CONCORDSYSTEM_API FConcordMetasoundTrackerModuleAsset
    {
    public:
        FConcordMetasoundTrackerModuleAsset() = default;
        FConcordMetasoundTrackerModuleAsset(const FConcordMetasoundTrackerModuleAsset&) = default;
        FConcordMetasoundTrackerModuleAsset& operator=(const FConcordMetasoundTrackerModuleAsset& Other) = default;
        FConcordMetasoundTrackerModuleAsset(const TUniquePtr<Audio::IProxyData>& InInitData);

        bool IsInitialized() const { return TrackerModuleProxy.IsValid(); }
        const FConcordTrackerModuleProxy* GetProxy() const { return TrackerModuleProxy.Get(); }
    private:
        TSharedPtr<const FConcordTrackerModuleProxy> TrackerModuleProxy;
    };

    DECLARE_METASOUND_DATA_REFERENCE_TYPES(FConcordMetasoundTrackerModuleAsset, CONCORDSYSTEM_API, FConcordMetasoundTrackerModuleAssetTypeInfo, FConcordMetasoundTrackerModuleAssetReadRef, FConcordMetasoundTrackerModuleAssetWriteRef)
}
