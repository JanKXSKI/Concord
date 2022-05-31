// Copyright Jan Klimaschewski. All Rights Reserved.

#include "ConcordModelGraphParameter.h"
#include "SConcordModelGraphInOutNode.h"

void UConcordModelGraphParameter::GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
    if (ContextMenuBuilder.FromPin && ContextMenuBuilder.FromPin->PinType.PinCategory == "Viewer") return;
    ContextMenuBuilder.AddAction(MakeShared<FConcordModelGraphAddNodeAction_NewParameter>(EConcordValueType::Int));
    ContextMenuBuilder.AddAction(MakeShared<FConcordModelGraphAddNodeAction_NewParameter>(EConcordValueType::Float));
}

void UConcordModelGraphParameter::OnRenameNode(const FString& NewNameString)
{
    UConcordModelGraphNode::OnRenameNode(NewNameString);
    OnInputOutputInterfaceChanged();
}

void UConcordModelGraphParameter::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    UConcordModelGraphNode::PostEditChangeProperty(PropertyChangedEvent);
    if (PropertyChangedEvent.GetPropertyName() == "bLocal")
    {
        GetParameter()->Modify();
        GetParameter()->bLocal = bLocal;
    }
    if (PropertyChangedEvent.Property) // not set when importing from text, in which case interface change is already handled
    {
        OnInputOutputInterfaceChanged();
    }
}

void UConcordModelGraphParameter::AllocateDefaultPins()
{
    UConcordModelGraphConsumer::AllocateDefaultPins();
    CreatePin(EGPD_Output, "Expression", "Parameter");
    OnInputOutputInterfaceChanged();
}

TSharedPtr<SGraphNode> UConcordModelGraphParameter::CreateVisualWidget()
{
    return SNew(SConcordModelGraphInOutNode, this)
    .bWithDefaultValues(false)
    .bWithOutputLabels(true)
    .bTitleOnTop(true);
}

void UConcordModelGraphParameter::PostEditImport()
{
    UConcordModelGraphNode::PostEditImport();
    OnInputOutputInterfaceChanged();
}

void UConcordModelGraphParameter::DestroyNode()
{
    GetModel()->Modify();
    GetModel()->Parameters.Remove(Name);
    GetModel()->OrderedParameterNames.RemoveSingle(Name);
    GetModel()->Parameters.Shrink();
    UConcordModelGraphNode::DestroyNode();
    OnInputOutputInterfaceChanged();
}

FConcordModelGraphAddNodeAction_NewParameter::FConcordModelGraphAddNodeAction_NewParameter()
    : FConcordModelGraphAddNodeAction_NewParameter(EConcordValueType::Int)
{}

FConcordModelGraphAddNodeAction_NewParameter::FConcordModelGraphAddNodeAction_NewParameter(EConcordValueType InType)
    : FConcordModelGraphAddNodeAction(FText(),
                                      FText::Format(INVTEXT("Parameter ({0})"), InType == EConcordValueType::Int ? INVTEXT("Int") : INVTEXT("Float")),
                                      Cast<UConcordVertex>(UConcordParameter::StaticClass()->GetDefaultObject())->GetTooltip(),
                                      INVTEXT("Add New Concord Parameter"))
    , Type(InType)
{}

UEdGraphNode* FConcordModelGraphAddNodeAction_NewParameter::MakeNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin)
{
    FGraphNodeCreator<UConcordModelGraphParameter> NodeCreator(*ParentGraph);
    UConcordModelGraphParameter* NewGraphParameter = NodeCreator.CreateUserInvokedNode();

    switch (Type)
    {
    case EConcordValueType::Int: NewGraphParameter->Vertex = NewObject<UConcordParameterInt>(NewGraphParameter->GetModel(), NAME_None, RF_Transactional); break;
    case EConcordValueType::Float: NewGraphParameter->Vertex = NewObject<UConcordParameterFloat>(NewGraphParameter->GetModel(), NAME_None, RF_Transactional); break;
    default: checkNoEntry();
    }

    NewGraphParameter->Name = FName("NewParameter");
    NewGraphParameter->SyncModelName(false);
    NodeCreator.Finalize();
    return NewGraphParameter;
}
