// Copyright Jan Klimaschewski. All Rights Reserved.

#include "ConcordModelGraphConsumer.h"
#include "ConcordModel.h"
#include "ConcordVertex.h"
#include "ConcordModelGraphSync.h"

FText UConcordModelGraphConsumer::GetPinDisplayName(const UEdGraphPin* Pin) const
{
    if (Pin->Direction == EGPD_Output) return FText::FromName(Pin->PinName);
    int32 Index = Pins.Find(const_cast<UEdGraphPin*>(Pin));
    check(Index != INDEX_NONE && Vertex);
    check(Index < Vertex->GetInputInfo().Num());
    return Vertex->GetInputInfo()[Index].DisplayName;
}

void UConcordModelGraphConsumer::AllocateDefaultPins()
{
    TArray<UConcordVertex::FInputInfo> InputInfo = Vertex->GetInputInfo();
    for (int32 PinIndex = 0; PinIndex < InputInfo.Num(); ++PinIndex)
    {
        CreatePin(EGPD_Input, "Expression", FName("VertexPin", PinIndex));
        Pins.Last()->DefaultValue = InputInfo[PinIndex].Default;
    }
    Vertex->ConnectedTransformers.SetNum(Pins.Num());
    FConcordModelGraphSync::SyncModelConnections(this);
    SetupShapeAndType();
}

void UConcordModelGraphConsumer::PinDefaultValueChanged(UEdGraphPin* Pin)
{
    int32 Index = Pins.Find(Pin);
    check(Index != INDEX_NONE && Vertex);
    FConcordModelGraphSync::SyncModelConnections(this, Index);
    GetGraph()->SetupShapesAndTypes();
}

void UConcordModelGraphConsumer::NodeConnectionListChanged()
{
    FConcordModelGraphSync::SyncModelConnections(this);
    GetGraph()->SetupShapesAndTypes();
}

void UConcordModelGraphConsumer::PostPasteNode()
{
    FConcordModelGraphSync::SyncModelConnections(this);
}

void UConcordModelGraphConsumer::SyncModelConnections()
{
    FConcordModelGraphSync::SyncModelConnections(this);
}
