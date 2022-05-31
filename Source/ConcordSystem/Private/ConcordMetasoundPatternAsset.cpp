// Copyright Jan Klimaschewski. All Rights Reserved.

#include "ConcordMetasoundPatternAsset.h"

using namespace Metasound;

FConcordMetasoundPatternAsset::FConcordMetasoundPatternAsset(const TUniquePtr<Audio::IProxyData>& InInitData)
{
    if (!InInitData.IsValid()) return;
    PatternProxy = MakeShared<const FConcordPatternProxy>(InInitData->GetAs<FConcordPatternProxy>());
}

DEFINE_METASOUND_DATA_TYPE(Metasound::FConcordMetasoundPatternAsset, "ConcordMetasoundPatternAsset")
