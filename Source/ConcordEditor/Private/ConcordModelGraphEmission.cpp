// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "ConcordModelGraphEmission.h"
#include "ConcordModel.h"
#include "ConcordEmission.h"
#include "Styling/SlateStyleRegistry.h"
#include "UObject/UObjectIterator.h"

void UConcordModelGraphEmission::GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
    if (ContextMenuBuilder.FromPin &&
        ContextMenuBuilder.FromPin->PinName != "All") return;
    ContextMenuBuilder.AddAction(MakeShared<FConcordModelGraphAddNodeAction_NewEmission>());
}

FText UConcordModelGraphEmission::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    return Vertex->GetDisplayName();
}

TSharedPtr<SGraphNode> UConcordModelGraphEmission::CreateVisualWidget()
{
    return SNew(SConcordModelGraphEmission, this);
}

void UConcordModelGraphEmission::PostEditImport()
{
    UConcordModelGraphConsumer::PostEditImport();
    GetModel()->Modify();
    GetModel()->Emissions.Add(GetEmission());
}

void UConcordModelGraphEmission::DestroyNode()
{
    GetModel()->Modify();
    GetModel()->Emissions.Remove(GetEmission());
    GetModel()->Emissions.Shrink();
    UEdGraphNode::DestroyNode();
}

void UConcordModelGraphEmission::AllocateDefaultPins()
{
    UConcordModelGraphConsumer::AllocateDefaultPins();
    for (UEdGraphPin* Pin : Pins) Pin->PinType.PinCategory = "Emission";
}

void SConcordModelGraphEmission::AddSinkDelegates()
{
    Cast<UConcordModelGraphNode>(GraphNode)->OnNodeChanged.AddSP(this, &SConcordModelGraphEmission::UpdateGraphNode);
}

FConcordModelGraphAddNodeAction_NewEmission::FConcordModelGraphAddNodeAction_NewEmission()
    : FConcordModelGraphAddNodeAction(FText(),
                                      INVTEXT("Emission"),
                                      Cast<UConcordVertex>(UConcordEmission::StaticClass()->GetDefaultObject())->GetTooltip(),
                                      INVTEXT("Add new Concord Emission"))
{}

UEdGraphNode* FConcordModelGraphAddNodeAction_NewEmission::MakeNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin)
{
    FGraphNodeCreator<UConcordModelGraphEmission> NodeCreator(*ParentGraph);
    UConcordModelGraphEmission* NewEmissionNode = NodeCreator.CreateUserInvokedNode();
    NewEmissionNode->Vertex = NewObject<UConcordEmission>(NewEmissionNode->GetModel(), NAME_None, RF_Transactional);
    NewEmissionNode->GetModel()->Emissions.Add(NewEmissionNode->GetEmission());
    NodeCreator.Finalize();
    return NewEmissionNode;
}
