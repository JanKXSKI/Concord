// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MetasoundNode.h"
#include "MetasoundBuilderInterface.h"
#include "MetasoundExecutableOperator.h"
#include "MetasoundPrimitives.h"
#include "MetasoundTrigger.h"

namespace Metasound
{
    class CONCORDSYSTEM_API FConcordClockNode : public FNode
    {
        class FOperatorFactory : public IOperatorFactory
        {
            virtual TUniquePtr<IOperator> CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors) override;
        };
    public:
        static const FVertexInterface& DeclareVertexInterface();
        static const FNodeClassMetadata& GetNodeInfo();

        FConcordClockNode(const FVertexName& InName, const FGuid& InInstanceID);
        FConcordClockNode(const FNodeInitData& InInitData);
        virtual ~FConcordClockNode() = default;

        virtual FOperatorFactorySharedRef GetDefaultOperatorFactory() const override { return Factory; }
        virtual const FVertexInterface& GetVertexInterface() const override { return Interface; }
        virtual bool SetVertexInterface(const FVertexInterface& InInterface) override { return InInterface == Interface; }
        virtual bool IsVertexInterfaceSupported(const FVertexInterface& InInterface) const override { return InInterface == Interface; }
    private:
        FOperatorFactorySharedRef Factory;
        FVertexInterface Interface;
    };

    class CONCORDSYSTEM_API FConcordClockOperator : public TExecutableOperator<FConcordClockOperator>
    {	
    public:
        FConcordClockOperator(const FOperatorSettings& InSettings,
                              const FTriggerReadRef& InStart,
                              const FTriggerReadRef& InStop,
                              const FFloatReadRef& InBPM,
                              const FInt32ReadRef& InLinesPerBeat,
                              const FInt32ReadRef& InStartLine,
                              const FInt32ReadRef& InNumberOfLines,
                              const FFloatReadRef& InShuffle,
                              const FBoolReadRef& bInLoop)
            : Settings(InSettings)
            , Start(InStart)
            , Stop(InStop)
            , BPM(InBPM)
            , LinesPerBeat(InLinesPerBeat)
            , StartLine(InStartLine)
            , NumberOfLines(InNumberOfLines)
            , Shuffle(InShuffle)
            , bLoop(bInLoop)
            , OnLine(FTriggerWriteRef::CreateNew(InSettings))
            , Index(FInt32WriteRef::CreateNew(0))
            , Alpha(FFloatWriteRef::CreateNew(0.0f))
            , OnStart(FTriggerWriteRef::CreateNew(InSettings))
            , Position(0.0)
            , bRunning(false)
        {}

        virtual FDataReferenceCollection GetInputs() const override;
        virtual FDataReferenceCollection GetOutputs() const override;

        void Execute();
    private:
        const FOperatorSettings Settings;

        FTriggerReadRef Start;
        FTriggerReadRef Stop;
        FFloatReadRef BPM;
        FInt32ReadRef LinesPerBeat;
        FInt32ReadRef StartLine;
        FInt32ReadRef NumberOfLines;
        FFloatReadRef Shuffle;
        FBoolReadRef bLoop;

        FTriggerWriteRef OnLine;
        FInt32WriteRef Index;
        FFloatWriteRef Alpha;
        FTriggerWriteRef OnStart;

        double Position;
        bool bRunning;

        double GetPositionFrameDelta() const;
        bool CheckNextLine(int32& CheckFromFrame, double PositionFrameDelta);
        void HandleLine(int32 Frame, int32 InIndex);
        int32 ShuffleFrame(int32 Frame, int32 InIndex) const;
    };
}
