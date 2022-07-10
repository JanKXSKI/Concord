// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "ConcordMetasoundGetColumn.h"
#include "MetasoundLog.h"

using namespace Metasound;

TUniquePtr<IOperator> FConcordGetColumnNode::FOperatorFactory::CreateOperator(const FBuildOperatorParams& InParams, 
                                                                              FBuildResults& OutResults)
{
    const FInputVertexInterfaceData& Inputs = InParams.InputData;
    return MakeUnique<FConcordGetColumnOperator>(Inputs.GetOrCreateDefaultDataReadReference<FTrigger>("Trigger", InParams.OperatorSettings),
                                                 Inputs.GetOrCreateDefaultDataReadReference<FConcordMetasoundPatternAsset>("Pattern", InParams.OperatorSettings),
                                                 Inputs.GetOrCreateDefaultDataReadReference<FString>("Column Path", InParams.OperatorSettings),
                                                 Inputs.GetOrCreateDefaultDataReadReference<int32>("Column Index Override", InParams.OperatorSettings));
}

const FVertexInterface& FConcordGetColumnNode::DeclareVertexInterface()
{
    static const FVertexInterface VertexInterface(FInputVertexInterface(TInputDataVertex<FTrigger>("Trigger", { INVTEXT("Trigger an update of the output column."), INVTEXT("Trigger") }),
                                                                        TInputDataVertex<FConcordMetasoundPatternAsset>("Pattern", { INVTEXT("Pattern to read from."), INVTEXT("Pattern") }),
                                                                        TInputDataVertex<FString>("Column Path", { INVTEXT("The path to the column."), INVTEXT("Column Path") }, FString(TEXT("Kick/Notes"))),
                                                                        TInputDataVertex<int32>("Column Index Override", { INVTEXT("Overrides the column index given in the column path if the override index is non-negative."), INVTEXT("Column Index Override") }, -1)),
                                                  FOutputVertexInterface(TOutputDataVertex<TArray<int32>>("Column", { INVTEXT("The column."), INVTEXT("Column") })));
    return VertexInterface;
}

const FNodeClassMetadata& FConcordGetColumnNode::GetNodeInfo()
{
    auto InitNodeInfo = []() -> FNodeClassMetadata
    {
        FNodeClassMetadata Info;
        Info.ClassName = { "Concord", "Get Column", "Default" };
        Info.MajorVersion = 1;
        Info.MinorVersion = 0;
        Info.DisplayName = INVTEXT("Concord Get Column");
        Info.Description = INVTEXT("Gets a column from a Concord pattern.");
        Info.Author = TEXT("Jan Klimaschewski");
        Info.PromptIfMissing = INVTEXT("Missing :(");
        Info.DefaultInterface = DeclareVertexInterface();

        return Info;
    };

    static const FNodeClassMetadata Info = InitNodeInfo();

    return Info;
}

FConcordGetColumnNode::FConcordGetColumnNode(const FVertexName& InName, const FGuid& InInstanceID)
    :	FNode(InName, InInstanceID, GetNodeInfo())
    ,	Factory(MakeOperatorFactoryRef<FConcordGetColumnNode::FOperatorFactory>())
    ,	Interface(DeclareVertexInterface())
{}
FConcordGetColumnNode::FConcordGetColumnNode(const FNodeInitData& InInitData)
    : FConcordGetColumnNode(InInitData.InstanceName, InInitData.InstanceID)
{}

FDataReferenceCollection FConcordGetColumnOperator::GetInputs() const
{
    FDataReferenceCollection InputDataReferences;
    InputDataReferences.AddDataReadReference("Trigger", Trigger);
    InputDataReferences.AddDataReadReference("Pattern", PatternAsset);
    InputDataReferences.AddDataReadReference("Column Path", ColumnPath);
    InputDataReferences.AddDataReadReference("Column Index Override", ColumnIndexOverride);
    return InputDataReferences;
}

FDataReferenceCollection FConcordGetColumnOperator::GetOutputs() const
{
    FDataReferenceCollection OutputDataReferences;
    OutputDataReferences.AddDataReadReference("Column", Column);
    return OutputDataReferences;
}

void FConcordGetColumnOperator::Execute()
{
    if (!Trigger->IsTriggeredInBlock() || !PatternAsset->IsInitialized()) return;
    const TOptional<FConcordColumnPath> OptionalPath = FConcordColumnPath::Parse(*ColumnPath);
    if (!OptionalPath) { UE_LOG(LogMetaSound, Error, TEXT("Concord Get Column: Invalid Column path: %s"), **ColumnPath); return; }
    const FConcordColumnPath& Path = OptionalPath.GetValue();
    const FConcordTrack* FoundTrack = PatternAsset->GetTracks().FindByHash(GetTypeHash(Path.TrackName), Path.TrackName);
    if (!FoundTrack) { UE_LOG(LogMetaSound, Error, TEXT("Concord Get Column: Track in pattern %s is not part of the queried pattern."), **ColumnPath); return; }
    int32 ColumnIndex = Path.ColumnIndex;
    if (*ColumnIndexOverride >= 0) ColumnIndex = *ColumnIndexOverride;
    if (ColumnIndex >= FoundTrack->Columns.Num()) { UE_LOG(LogMetaSound, Error, TEXT("Concord Get Column: Column index %i out of range in %s."), ColumnIndex, **ColumnPath); return; }
    switch (Path.ColumnValuesType)
    {
    case EConcordColumnValuesType::Note: SetColumn(FoundTrack->Columns[ColumnIndex].NoteValues); break;
    case EConcordColumnValuesType::Instrument: SetColumn(FoundTrack->Columns[ColumnIndex].InstrumentValues); break;
    case EConcordColumnValuesType::Volume: SetColumn(FoundTrack->Columns[ColumnIndex].VolumeValues); break;
    case EConcordColumnValuesType::Delay: SetColumn(FoundTrack->Columns[ColumnIndex].DelayValues); break;
    }
}

void FConcordGetColumnOperator::SetColumn(const TArray<int32>& Values)
{
    *Column = Values;
    if (Values.IsEmpty()) { UE_LOG(LogMetaSound, Warning, TEXT("Concord Get Column: Set empty column at %s"), **ColumnPath); }
}
