// Copyright Jan Klimaschewski. All Rights Reserved.

#include "ConcordMetasoundClock.h"

using namespace Metasound;

TUniquePtr<IOperator> FConcordClockNode::FOperatorFactory::CreateOperator(const FCreateOperatorParams& InParams, 
                                                                          FBuildErrorArray& OutErrors) 
{
    const FConcordClockNode& Node = static_cast<const FConcordClockNode&>(InParams.Node);
    const FDataReferenceCollection& Inputs = InParams.InputDataReferences;
    const FInputVertexInterface& InputInterface = DeclareVertexInterface().GetInputInterface();

    return MakeUnique<FConcordClockOperator>(InParams.OperatorSettings,
                                             Inputs.GetDataReadReferenceOrConstruct<FTrigger>("Start", InParams.OperatorSettings),
                                             Inputs.GetDataReadReferenceOrConstruct<FTrigger>("Stop", InParams.OperatorSettings),
                                             Inputs.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, "BPM", InParams.OperatorSettings),
                                             Inputs.GetDataReadReferenceOrConstructWithVertexDefault<int32>(InputInterface, "Lines per Beat", InParams.OperatorSettings),
                                             Inputs.GetDataReadReferenceOrConstructWithVertexDefault<int32>(InputInterface, "Start Line", InParams.OperatorSettings),
                                             Inputs.GetDataReadReferenceOrConstructWithVertexDefault<int32>(InputInterface, "Number of Lines", InParams.OperatorSettings),
                                             Inputs.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, "Shuffle", InParams.OperatorSettings),
                                             Inputs.GetDataReadReferenceOrConstructWithVertexDefault<bool>(InputInterface, "Loop", InParams.OperatorSettings));
}

const FVertexInterface& FConcordClockNode::DeclareVertexInterface()
{
    static const FVertexInterface VertexInterface(FInputVertexInterface(TInputDataVertexModel<FTrigger>("Start", INVTEXT("Start the Clock.")),
                                                                        TInputDataVertexModel<FTrigger>("Stop", INVTEXT("Stop the Clock.")),
                                                                        TInputDataVertexModel<float>("BPM", INVTEXT("BPM."), 120.0f),
                                                                        TInputDataVertexModel<int32>("Lines per Beat", INVTEXT("Number of lines for that make up one beat."), 4),
                                                                        TInputDataVertexModel<int32>("Start Line", INVTEXT("The line to start the clock at."), 0),
                                                                        TInputDataVertexModel<int32>("Number of Lines", INVTEXT("Number of lines in the pattern."), 32),
                                                                        TInputDataVertexModel<float>("Shuffle", INVTEXT("Moves every second line towards the next."), 0.0f),
                                                                        TInputDataVertexModel<bool>("Loop", INVTEXT("Loop the Clock instead of stopping when finished."), true)),
                                                  FOutputVertexInterface(TOutputDataVertexModel<FTrigger>("On Line", INVTEXT("On line trigger.")),
                                                                         TOutputDataVertexModel<int32>("Index", INVTEXT("Index.")),
                                                                         TOutputDataVertexModel<float>("Alpha", INVTEXT("Alpha (between 0 and 1).")),
                                                                         TOutputDataVertexModel<FTrigger>("On Start", INVTEXT("On start trigger (useful for syncing when looping)."))));
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
