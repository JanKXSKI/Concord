// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "MetasoundSource.h"
#include "MetasoundNodeRegistrationMacro.h"
#include "MetasoundDataTypeRegistrationMacro.h"
#include "ConcordPattern.h"
#include "ConcordTrackerModule.h"
#include "ConcordMetasoundTrackerModulePlayer.h"
#include "ConcordMetasoundClock.h"
#include "ConcordMetasoundGetColumn.h"
#include "ConcordPreviewAudioParameterInterface.h"
#include "IAudioParameterInterfaceRegistry.h"

class FConcordSystemModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        Metasound::RegisterDataTypeWithFrontend<Metasound::FConcordMetasoundPatternAsset, Metasound::ELiteralType::UObjectProxy, UConcordPattern>();
        Metasound::RegisterDataTypeWithFrontend<Metasound::FConcordMetasoundTrackerModuleAsset, Metasound::ELiteralType::UObjectProxy, UConcordTrackerModule>();
        Metasound::RegisterNodeWithFrontend<Metasound::FConcordTrackerModulePlayerNode>();
        Metasound::RegisterNodeWithFrontend<Metasound::FConcordClockNode>();
        Metasound::RegisterNodeWithFrontend<Metasound::FConcordGetColumnNode>();

        Audio::IAudioParameterInterfaceRegistry::Get().RegisterInterface(MakeShared<FConcordPreviewAudioParameterInterface>(*UMetaSoundSource::StaticClass()));
    }

    virtual void ShutdownModule() override
    {

    }
};

IMPLEMENT_MODULE(FConcordSystemModule, ConcordSystem);
