// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "ConcordMetasoundTrackerModuleAsset.h"

using namespace Metasound;

FConcordMetasoundTrackerModuleAsset::FConcordMetasoundTrackerModuleAsset(const TUniquePtr<Audio::IProxyData>& InInitData)
{
    if (!InInitData.IsValid()) return;
    TrackerModuleProxy = MakeShared<const FConcordTrackerModuleProxy>(InInitData->GetAs<FConcordTrackerModuleProxy>());
}

DEFINE_METASOUND_DATA_TYPE(Metasound::FConcordMetasoundTrackerModuleAsset, "ConcordMetasoundTrackerModuleAsset")
