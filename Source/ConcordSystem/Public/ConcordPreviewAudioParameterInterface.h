// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IAudioParameterInterfaceRegistry.h"
#include "MetasoundDataReference.h"
#include "MetasoundTrigger.h"
#include "ConcordMetasoundPatternAsset.h"

#define LOCTEXT_NAMESPACE "MetasoundEngine"
namespace Metasound::Engine
{
	namespace InputFormatPrivate
	{
		TArray<Audio::FParameterInterface::FClassOptions> GetUClassOptions()
		{
			return
			{
				{ UMetaSoundPatch::StaticClass()->GetClassPathName(), true /* bIsModifiable */, false /* bIsDefault */ },
				{ UMetaSoundSource::StaticClass()->GetClassPathName(), false /* bIsModifiable */ , false /* bIsDefault */ }
			};
		};
	} // namespace InputFormatPrivate


#define AUDIO_PARAMETER_INTERFACE_NAMESPACE "Concord.Preview"
	namespace FConcordPreviewAudioParameterInterface
	{
		const FMetasoundFrontendVersion& GetVersion()
		{
			static const FMetasoundFrontendVersion Version = { AUDIO_PARAMETER_INTERFACE_NAMESPACE, { 1, 0 } };
			return Version;
		}

		namespace Inputs
		{
			const FName MonoIn = AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE("Audio:0");
		} // namespace Inputs

		Audio::FParameterInterfacePtr CreateInterface()
		{
			struct FInterface : public Audio::FParameterInterface
			{


				FInterface() : Audio::FParameterInterface("Concord", { 1, 0 })
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
						},
						{
							INVTEXT("Concord.BPM"),
							INVTEXT("The playback speed in beats per minute."),
							Metasound::GetMetasoundDataTypeName<int32>(),
							{ "Concord.BPM", 120 }
						},
						{
							INVTEXT("Concord.LinesPerBeat"),
							INVTEXT("The number of lines that makes up a beat."),
							Metasound::GetMetasoundDataTypeName<int32>(),
							{ "Concord.LinesPerBeat", 4 }
						},
						{
							INVTEXT("Concord.NumberOfLines"),
							INVTEXT("The number of lines to play."),
							Metasound::GetMetasoundDataTypeName<int32>(),
							{ "Concord.NumberOfLines", 32 }
						}
					};


				}
			};
			return MakeShared<FInterface>();
		}
	}

}
