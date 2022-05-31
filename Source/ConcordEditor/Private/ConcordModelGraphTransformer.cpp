// Copyright Jan Klimaschewski. All Rights Reserved.

#include "ConcordModelGraphTransformer.h"
#include "SConcordModelGraphInOutNode.h"
#include "ConcordModel.h"
#include "ConcordBox.h"
#include "ConcordParameter.h"
#include "ConcordComposite.h"
#include "ConcordInstance.h"
#include "ConcordFactor.h"
#include "UObject/UObjectIterator.h"

void UConcordModelGraphTransformer::GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
    if (ContextMenuBuilder.FromPin &&
        ContextMenuBuilder.FromPin->PinType.PinCategory == "Viewer") return;

    TArray<UEdGraphPin*> SourcePins = Cast<UConcordModelGraph>(ContextMenuBuilder.CurrentGraph)->GetSelectedPins(EGPD_Output);

    for (TObjectIterator<UClass> It; It; ++It)
    {
        if (!It->IsChildOf(UConcordTransformer::StaticClass())
            || !It->IsNative()
            || It->HasAnyClassFlags(EClassFlags::CLASS_Abstract)
            || It->IsChildOf(UConcordFactor::StaticClass())
            || *It == UConcordBox::StaticClass()
            || *It == UConcordBoxStateSet::StaticClass()
            || *It == UConcordParameterInt::StaticClass()
            || *It == UConcordParameterFloat::StaticClass()
            || *It == UConcordCompositeOutput::StaticClass()
            || *It == UConcordInstanceOutput::StaticClass())
            continue;
        ContextMenuBuilder.AddAction(MakeShared<FConcordModelGraphAddNodeAction_NewTransformer>(*It, SourcePins));
    }
}

FText UConcordModelGraphTransformer::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    return Vertex->GetNodeDisplayName().ToUpper();
}

TSharedPtr<SGraphNode> UConcordModelGraphTransformer::CreateVisualWidget()
{
    return SNew(SConcordModelGraphInOutNode, this);
}

void UConcordModelGraphTransformer::AllocateDefaultPins()
{
    UConcordModelGraphConsumer::AllocateDefaultPins();
    CreatePin(EGPD_Output, "Expression", "TransformerOutputPin");
}

FConcordModelGraphAddNodeAction_NewTransformer::FConcordModelGraphAddNodeAction_NewTransformer()
    : FConcordModelGraphAddNodeAction_NewTransformer(nullptr, {})
{}

FConcordModelGraphAddNodeAction_NewTransformer::FConcordModelGraphAddNodeAction_NewTransformer(TSubclassOf<UConcordTransformer> InClass, const TArray<UEdGraphPin*>& InSourcePins)
    : FConcordModelGraphAddNodeAction(InClass ? FText::Format(INVTEXT("Transformers|{0}"), FText::FromString(InClass->GetDefaultObject<UConcordTransformer>()->GetCategory())) : FText(),
                                      InClass ? InClass->GetDefaultObject<UConcordTransformer>()->GetDisplayName() : INVTEXT("NONE"),
                                      InClass ? InClass->GetDefaultObject<UConcordTransformer>()->GetTooltip() : INVTEXT("NONE"),
                                      INVTEXT("Add New Concord Transformer"))
    , Class(InClass)
    , SourcePins(InSourcePins)
{}

UEdGraphNode* FConcordModelGraphAddNodeAction_NewTransformer::MakeNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin)
{
    FGraphNodeCreator<UConcordModelGraphTransformer> NodeCreator(*ParentGraph);
    UConcordModelGraphTransformer* NewGraphTransformer = NodeCreator.CreateUserInvokedNode();
    NewGraphTransformer->Vertex = NewObject<UConcordTransformer>(NewGraphTransformer->GetModel(), Class, NAME_None, RF_Transactional);

    UConcordAdaptiveTransformer* AdaptiveTransformer = Cast<UConcordAdaptiveTransformer>(NewGraphTransformer->Vertex);
    if (AdaptiveTransformer && SourcePins.Num() > 1 && !FromPin)
    {
        AdaptiveTransformer->NumInputs = SourcePins.Num();
        NodeCreator.Finalize();
        for (int32 PinIndex = 0; PinIndex < SourcePins.Num(); ++PinIndex)
        {
            ParentGraph->GetSchema()->TryCreateConnection(SourcePins[PinIndex], NewGraphTransformer->GetPinWithDirectionAt(PinIndex, EGPD_Input));
            SourcePins[PinIndex]->GetOwningNode()->NodeConnectionListChanged();
        }
        NewGraphTransformer->NodeConnectionListChanged();
    }
    else NodeCreator.Finalize();
    return NewGraphTransformer;
}
