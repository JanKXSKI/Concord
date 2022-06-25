// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Guid.h"
#include "ConcordMetasoundTrackerModuleAsset.h"
#include "ConcordMetasoundPatternAsset.h"
#include "MetasoundNode.h"
#include "MetasoundBuilderInterface.h"
#include "MetasoundExecutableOperator.h"
#include "MetasoundPrimitives.h"
#include "MetasoundTrigger.h"
#include "MetasoundAudioBuffer.h"
#include "xmp.h"

namespace Metasound
{
    class CONCORDSYSTEM_API FConcordTrackerModulePlayerNode : public FNode
    {
        class FOperatorFactory : public IOperatorFactory
        {
            virtual TUniquePtr<IOperator> CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors) override;
        };
    public:
        static const FVertexInterface& DeclareVertexInterface();
        static const FNodeClassMetadata& GetNodeInfo();

        FConcordTrackerModulePlayerNode(const FVertexName& InName, const FGuid& InInstanceID);
        FConcordTrackerModulePlayerNode(const FNodeInitData& InInitData);
        virtual ~FConcordTrackerModulePlayerNode() = default;

        virtual FOperatorFactorySharedRef GetDefaultOperatorFactory() const override { return Factory; }
        virtual const FVertexInterface& GetVertexInterface() const override { return Interface; }
        virtual bool SetVertexInterface(const FVertexInterface& InInterface) override { return InInterface == Interface; }
        virtual bool IsVertexInterfaceSupported(const FVertexInterface& InInterface) const override { return InInterface == Interface; }
    private:
        FOperatorFactorySharedRef Factory;
        FVertexInterface Interface;
    };

    class CONCORDSYSTEM_API FConcordTrackerModulePlayerOperator : public TExecutableOperator<FConcordTrackerModulePlayerOperator>
    {	
    public:
        FConcordTrackerModulePlayerOperator(const FOperatorSettings& InSettings,
                               const FConcordMetasoundTrackerModuleAssetReadRef& InTrackerModuleAsset,
                               const FConcordMetasoundPatternAssetReadRef& InPatternAsset,
                               const FTriggerReadRef& InStart,
                               const FTriggerReadRef& InStop,
                               const FInt32ReadRef& InBPM,
                               const FInt32ReadRef& InLinesPerBeat,
                               const FInt32ReadRef& InStartLine,
                               const FInt32ReadRef& InNumberOfLines,
                               const FBoolReadRef& bInLoop)
            : Settings(InSettings)
            , context(nullptr)
            , TrackerModuleAsset(InTrackerModuleAsset)
            , PatternAsset(InPatternAsset)
            , Start(InStart)
            , Stop(InStop)
            , BPM(InBPM)
            , LinesPerBeat(InLinesPerBeat)
            , StartLine(InStartLine)
            , NumberOfLines(InNumberOfLines)
            , bLoop(bInLoop)
            , LeftAudioOutput(FAudioBufferWriteRef::CreateNew(InSettings))
            , RightAudioOutput(FAudioBufferWriteRef::CreateNew(InSettings))
            , CurrentBPM(-1)
            , CurrentLinesPerBeat(-1)
            , InitialNumberOfLines(0)
            , CurrentNumberOfLines(-1)
            , bCleared(true)
        { XMPBuffer.SetNumUninitialized(Settings.GetNumFramesPerBlock() * 2); }
        ~FConcordTrackerModulePlayerOperator() { FreeXmp(); }

        virtual FDataReferenceCollection GetInputs() const override;
        virtual FDataReferenceCollection GetOutputs() const override;

        void Execute();
    private:
        const FOperatorSettings Settings;

        xmp_context context;
        xmp_module_info module_info;
        TArray<int16> XMPBuffer;

        FConcordMetasoundTrackerModuleAssetReadRef TrackerModuleAsset;
        FConcordMetasoundPatternAssetReadRef PatternAsset;
        FTriggerReadRef Start;
        FTriggerReadRef Stop;
        FInt32ReadRef BPM;
        FInt32ReadRef LinesPerBeat;
        FInt32ReadRef StartLine;
        FInt32ReadRef NumberOfLines;
        FBoolReadRef bLoop;

        FAudioBufferWriteRef LeftAudioOutput;
        FAudioBufferWriteRef RightAudioOutput;

        FGuid CurrentTrackerModuleGuid;
        FGuid CurrentPatternGuid;
        int32 CurrentBPM;
        int32 CurrentLinesPerBeat;
        int32 InitialNumberOfLines;
        int32 CurrentNumberOfLines;
        bool bCleared;

        bool ReinitXmp();
        bool LoadTrackerModule();
        void UpdatePattern();
        void UpdateBPM();
        void UpdateLinesPerBeat();
        int32 GetXmpRow() const;
        void CheckNumberOfLines();
        void ClearPattern();
        void SetPlayerStartPosition();
        void PlayModule(int32 StartFrame, int32 EndFrame);
        void FreeXmp();
    };
}
