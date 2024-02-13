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
#include "MetasoundUObjectRegistry.h"
#include "Misc/AssetRegistryInterface.h"
#include "Interfaces/MetasoundFrontendInterfaceRegistry.h"
#include "MetasoundFrontendDocument.h"
#include "MetasoundUObjectRegistry.h"



class FConcordSystemModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
       
        using namespace Metasound::Engine;

        Metasound::RegisterDataTypeWithFrontend<Metasound::FConcordMetasoundPatternAsset, Metasound::ELiteralType::UObjectProxy, UConcordPattern>();
        Metasound::RegisterDataTypeWithFrontend<Metasound::FConcordMetasoundTrackerModuleAsset, Metasound::ELiteralType::UObjectProxy, UConcordTrackerModule>();
        Metasound::RegisterNodeWithFrontend<Metasound::FConcordTrackerModulePlayerNode>();
        Metasound::RegisterNodeWithFrontend<Metasound::FConcordClockNode>();
        Metasound::RegisterNodeWithFrontend<Metasound::FConcordGetColumnNode>();


        Metasound::Frontend::IInterfaceRegistry& Reg = Metasound::Frontend::IInterfaceRegistry::Get();
        Audio::IAudioParameterInterfaceRegistry& InterfaceRegistry = Audio::IAudioParameterInterfaceRegistry::Get();
        InterfaceRegistry.RegisterInterface(FConcordPreviewAudioParameterInterface::CreateInterface());


    }

    virtual void ShutdownModule() override
    {

    }
};

IMPLEMENT_MODULE(FConcordSystemModule, ConcordSystem);
