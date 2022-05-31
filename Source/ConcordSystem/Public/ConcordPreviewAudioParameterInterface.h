// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IAudioParameterInterfaceRegistry.h"
#include "MetasoundDataReference.h"
#include "MetasoundTrigger.h"
#include "ConcordMetasoundPatternAsset.h"

struct FConcordPreviewAudioParameterInterface : public Audio::FParameterInterface
{
	FConcordPreviewAudioParameterInterface(const UClass& InAssetClass)
		: FParameterInterface("Concord", { 1, 0 }, InAssetClass)
	{
		Inputs =
		{
			{
				INVTEXT("Concord.Start"),
				INVTEXT("Starts the pattern."),
				Metasound::GetMetasoundDataTypeName<Metasound::FTrigger>(),
				{ "Concord.Start" }
			},
			{
				INVTEXT("Concord.StartLine"),
				INVTEXT("The line to start playing on."),
				Metasound::GetMetasoundDataTypeName<int32>(),
				{ "Concord.StartLine", 0 }
			},
			{
				INVTEXT("Concord.Pattern"),
				INVTEXT("The pattern to play."),
				Metasound::GetMetasoundDataTypeName<Metasound::FConcordMetasoundPatternAsset>(),
				{ "Concord.Pattern", (UObject*)nullptr }
			}
		};
	}
};
