// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "ConcordMetasoundClock.h"

using namespace Metasound;

TUniquePtr<IOperator> FConcordClockNode::FOperatorFactory::CreateOperator(const FBuildOperatorParams& InParams, 
                                                                          FBuildResults& OutResults) 
{
    const FInputVertexInterfaceData& Inputs = InParams.InputData;
    return MakeUnique<FConcordClockOperator>(InParams.OperatorSettings,
                                             Inputs.GetOrCreateDefaultDataReadReference<FTrigger>("Start", InParams.OperatorSettings),
                                             Inputs.GetOrCreateDefaultDataReadReference<FTrigger>("Stop", InParams.OperatorSettings),
                                             Inputs.GetOrCreateDefaultDataReadReference<float>("BPM", InParams.OperatorSettings),
                                             Inputs.GetOrCreateDefaultDataReadReference<int32>("Lines per Beat", InParams.OperatorSettings),
                                             Inputs.GetOrCreateDefaultDataReadReference<int32>("Start Line", InParams.OperatorSettings),
                                             Inputs.GetOrCreateDefaultDataReadReference<int32>("Number of Lines", InParams.OperatorSettings),
                                             Inputs.GetOrCreateDefaultDataReadReference<float>("Shuffle", InParams.OperatorSettings),
                                             Inputs.GetOrCreateDefaultDataReadReference<bool>("Loop", InParams.OperatorSettings));
}

const FVertexInterface& FConcordClockNode::DeclareVertexInterface()
{
    static const FVertexInterface VertexInterface(FInputVertexInterface(TInputDataVertex<FTrigger>("Start", { INVTEXT("Start the Clock."), INVTEXT("Start") }),
                                                                        TInputDataVertex<FTrigger>("Stop", { INVTEXT("Stop the Clock."), INVTEXT("Stop") }),
                                                                        TInputDataVertex<float>("BPM", { INVTEXT("BPM."), INVTEXT("BPM") }, 120.0f),
                                                                        TInputDataVertex<int32>("Lines per Beat", { INVTEXT("Number of lines for that make up one beat."), INVTEXT("Lines per Beat") }, 4),
                                                                        TInputDataVertex<int32>("Start Line", { INVTEXT("The line to start the clock at."), INVTEXT("Start Line") }, 0),
                                                                        TInputDataVertex<int32>("Number of Lines", { INVTEXT("Number of lines in the pattern."), INVTEXT("Number of Lines") }, 32),
                                                                        TInputDataVertex<float>("Shuffle", { INVTEXT("Moves every second line towards the next."), INVTEXT("Shuffle") }, 0.0f),
                                                                        TInputDataVertex<bool>("Loop", { INVTEXT("Loop the Clock instead of stopping when finished."), INVTEXT("Loop") }, true)),
                                                  FOutputVertexInterface(TOutputDataVertex<FTrigger>("On Line", { INVTEXT("On line trigger."), INVTEXT("On Line") }),
                                                                         TOutputDataVertex<int32>("Index", { INVTEXT("Index."), INVTEXT("Index") }),
                                                                         TOutputDataVertex<float>("Alpha", { INVTEXT("Alpha (between 0 and 1)."), INVTEXT("Alpha") }),
                                                                         TOutputDataVertex<FTrigger>("On Start", { INVTEXT("On start trigger (useful for syncing when looping)."), INVTEXT("On Start") })));
    return VertexInterface;
}

const FNodeClassMetadata& FConcordClockNode::GetNodeInfo()
{
    auto InitNodeInfo = []() -> FNodeClassMetadata
    {
        FNodeClassMetadata Info;
        Info.ClassName = { "Concord", "Clock", "Default" };
        Info.MajorVersion = 1;
        Info.MinorVersion = 0;
        Info.DisplayName = INVTEXT("Concord Clock");
        Info.Description = INVTEXT("Outputs timing for reading from Concord lines.");
        Info.Author = TEXT("Jan Klimaschewski");
        Info.PromptIfMissing = INVTEXT("Missing :(");
        Info.DefaultInterface = DeclareVertexInterface();

        return Info;
    };

    static const FNodeClassMetadata Info = InitNodeInfo();

    return Info;
}

FConcordClockNode::FConcordClockNode(const FVertexName& InName, const FGuid& InInstanceID)
    :	FNode(InName, InInstanceID, GetNodeInfo())
    ,	Factory(MakeOperatorFactoryRef<FConcordClockNode::FOperatorFactory>())
    ,	Interface(DeclareVertexInterface())
{}
FConcordClockNode::FConcordClockNode(const FNodeInitData& InInitData)
    : FConcordClockNode(InInitData.InstanceName, InInitData.InstanceID)
{}

FDataReferenceCollection FConcordClockOperator::GetInputs() const
{
    FDataReferenceCollection InputDataReferences;
    InputDataReferences.AddDataReadReference("Start", Start);
    InputDataReferences.AddDataReadReference("Stop", Stop);
    InputDataReferences.AddDataReadReference("BPM", BPM);
    InputDataReferences.AddDataReadReference("Lines per Beat", LinesPerBeat);
    InputDataReferences.AddDataReadReference("Start Line", StartLine);
    InputDataReferences.AddDataReadReference("Number of Lines", NumberOfLines);
    InputDataReferences.AddDataReadReference("Shuffle", Shuffle);
    InputDataReferences.AddDataReadReference("Loop", bLoop);
    return InputDataReferences;
}

FDataReferenceCollection FConcordClockOperator::GetOutputs() const
{
    FDataReferenceCollection OutputDataReferences;
    OutputDataReferences.AddDataReadReference("On Line", OnLine);
    OutputDataReferences.AddDataReadReference("Index", Index);
    OutputDataReferences.AddDataReadReference("Alpha", Alpha);
    OutputDataReferences.AddDataReadReference("On Start", OnStart);
    return OutputDataReferences;
}

void FConcordClockOperator::Execute()
{
    OnLine->AdvanceBlock();
    OnStart->AdvanceBlock();

    int32 CheckFromFrame = 0;
    Start->ExecuteBlock([](int32, int32){}, [&](int32 BeginFrame, int32 EndFrame)
    {
        Position = FMath::Min(*NumberOfLines - 1, *StartLine);
        bRunning = true;
        CheckFromFrame = BeginFrame;
    });

    if (Stop->IsTriggeredInBlock())
    {
        Position = 0.0;
        bRunning = false;
    }

    if (!bRunning || *BPM <= 0.0f) return;
    *Alpha = FMath::Frac(Position);

    const double PositionFrameDelta = GetPositionFrameDelta();
    while (CheckNextLine(CheckFromFrame, PositionFrameDelta));
}

double FConcordClockOperator::GetPositionFrameDelta() const
{
    const double SecondsPerLine = 60.0 / ((*BPM) * (*LinesPerBeat));
    const double SecondsPerFrame = 1.0 / Settings.GetSampleRate();
    return SecondsPerFrame / SecondsPerLine;
}

bool FConcordClockOperator::CheckNextLine(int32& CheckFromFrame, double PositionFrameDelta)
{
    int32 NextIndex = FMath::CeilToInt(Position);
    if (NextIndex >= *NumberOfLines)
    {
        if (!*bLoop) return false;
        Position -= NextIndex;
        NextIndex = 0;
    }

    const int32 NumFramesToNextLine = FMath::TruncToInt((NextIndex - Position) / PositionFrameDelta);
    const int32 NumFramesLeft = Settings.GetNumFramesPerBlock() - CheckFromFrame;
    if (NumFramesToNextLine >= NumFramesLeft)
    {
        Position += NumFramesLeft * PositionFrameDelta;
        return false;
    }
    CheckFromFrame += NumFramesToNextLine;
    HandleLine(CheckFromFrame, NextIndex);
    CheckFromFrame += 1;
    Position += (NumFramesToNextLine + 1) * PositionFrameDelta;
    return true;
}

void FConcordClockOperator::HandleLine(int32 Frame, int32 InIndex)
{
    OnLine->TriggerFrame(ShuffleFrame(Frame, InIndex));
    *Index = InIndex;
    if (InIndex == 0) OnStart->TriggerFrame(Frame);
}

int32 FConcordClockOperator::ShuffleFrame(int32 Frame, int32 InIndex) const
{
    if (InIndex % 2 == 0) return Frame;
    const float ShuffleFactor = FMath::GetMappedRangeValueClamped(TRange<float>(0.0f, 1.0f), TRange<float>(0.0f, 0.75f), *Shuffle);
    return Frame + int32(ShuffleFactor * (60.0f / ((*BPM) * (*LinesPerBeat))) * Settings.GetSampleRate());
}
