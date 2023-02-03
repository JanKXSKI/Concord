// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once
#include "Internationalization/Text.h"
#include "MetasoundExecutableOperator.h"
#include "MetasoundNodeRegistrationMacro.h"
#include "MetasoundParamHelper.h"
#include "MetasoundPrimitives.h"
#include "MetasoundStandardNodesNames.h"
#include "MetasoundStandardNodesCategories.h"
#include "MetasoundFacade.h"
#include "CoreMinimal.h"
#include "MetasoundNode.h"
#include "MetasoundBuilderInterface.h"
#include "MetasoundTrigger.h"
#include "MetasoundNodeInterface.h"
#include "MetasoundVertex.h"
#include <type_traits>

namespace Metasound
{
    class CONCORDSYSTEM_API FConcordClockNode : public FNodeFacade
    {

    public:
        
        FConcordClockNode(const FNodeInitData& InitData);
        virtual ~FConcordClockNode() = default;

        
    };

    class CONCORDSYSTEM_API FConcordClockOperator : public TExecutableOperator<FConcordClockOperator>
    {	
    public:

		static const FNodeClassMetadata& GetNodeInfo();
		static const FVertexInterface& GetVertexInterface();
		static TUniquePtr<IOperator> CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors);
        
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
