// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "ConcordModelGraphFactor.h"
#include "ConcordModel.h"
#include "ConcordFactor.h"
#include "Styling/SlateStyleRegistry.h"
#include "UObject/UObjectIterator.h"

void UConcordModelGraphFactor::GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
    if (ContextMenuBuilder.FromPin &&
        ContextMenuBuilder.FromPin->Direction != EGPD_Output) return;
    ContextMenuBuilder.AddAction(MakeShared<FConcordModelGraphAddNodeAction_NewFactor>());
}

FText UConcordModelGraphFactor::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    return Vertex->GetDisplayName();
}

TSharedPtr<SGraphNode> UConcordModelGraphFactor::CreateVisualWidget()
{
    return SNew(SConcordModelGraphFactor, this);
}

void UConcordModelGraphFactor::PostEditImport()
{
    UConcordModelGraphConsumer::PostEditImport();
    GetModel()->Modify();
    GetModel()->Factors.Add(GetFactor());
}

void UConcordModelGraphFactor::DestroyNode()
{
    GetModel()->Modify();
    GetModel()->Factors.Remove(GetFactor());
    GetModel()->Factors.Shrink();
    UEdGraphNode::DestroyNode();
}

void SConcordModelGraphFactor::AddSinkDelegates()
{
    Cast<UConcordModelGraphNode>(GraphNode)->OnNodeChanged.AddSP(this, &SConcordModelGraphFactor::UpdateGraphNode);
}

FConcordModelGraphAddNodeAction_NewFactor::FConcordModelGraphAddNodeAction_NewFactor()
    : FConcordModelGraphAddNodeAction(FText(),
                                      INVTEXT("Factor"),
                                      Cast<UConcordVertex>(UConcordFactor::StaticClass()->GetDefaultObject())->GetTooltip(),
                                      INVTEXT("Add new Concord Factor"))
{}

UEdGraphNode* FConcordModelGraphAddNodeAction_NewFactor::MakeNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin)
{
    FGraphNodeCreator<UConcordModelGraphFactor> NodeCreator(*ParentGraph);
    UConcordModelGraphFactor* NewFactorNode = NodeCreator.CreateUserInvokedNode();
    NewFactorNode->Vertex = NewObject<UConcordFactor>(NewFactorNode->GetModel(), NAME_None, RF_Transactional);
    NewFactorNode->GetModel()->Factors.Add(NewFactorNode->GetFactor());
    NodeCreator.Finalize();
    return NewFactorNode;
}
