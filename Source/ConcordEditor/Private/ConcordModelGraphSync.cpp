// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "ConcordModelGraphSync.h"
#include "ConcordModelGraphConsumer.h"
#include "ConcordModelGraphBox.h"
#include "ConcordModelGraphParameter.h"
#include "ConcordModelGraphTransformer.h"
#include "ConcordModelGraphModelNode.h"
#include "ConcordModel.h"
#include "ConcordBox.h"
#include "ConcordParameter.h"
#include "Transformers/ConcordTransformerValue.h"
#include "Transformers/ConcordTransformerCast.h"
#include "Transformers/ConcordTransformerElement.h"
#include "Transformers/ConcordTransformerConcat.h"

void FConcordModelGraphSync::SyncModelConnections(UConcordModelGraphConsumer* VertexNode)
{
    for (int32 PinIndex = 0; PinIndex < VertexNode->Vertex->GetInputInfo().Num(); ++PinIndex)
        SyncModelConnections(VertexNode, PinIndex);
}

void FConcordModelGraphSync::SyncModelConnections(UConcordModelGraphConsumer* VertexNode, int32 PinIndex)
{
    UConcordVertex* Vertex = VertexNode->Vertex;
    Vertex->Modify();
    UEdGraphPin* Pin = VertexNode->Pins[PinIndex];

    if (Pin->LinkedTo.Num() == 0)
    {
        Vertex->ConnectedTransformers[PinIndex] = MakeDefaultValue(Vertex, PinIndex, Pin);
    }

    else if (Pin->LinkedTo.Num() == 1)
    {
        Vertex->ConnectedTransformers[PinIndex] = GetOwningTransformer(Pin->LinkedTo[0]);
    }

    else
    {
        UConcordTransformerConcat* TransformerConcat = NewObject<UConcordTransformerConcat>(Vertex);
        for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
            TransformerConcat->ConnectedTransformers.Add(GetOwningTransformer(LinkedPin));
        Vertex->ConnectedTransformers[PinIndex] = TransformerConcat;
    }
}

UConcordTransformer* FConcordModelGraphSync::MakeDefaultValue(UConcordVertex* Vertex, int32 PinIndex, UEdGraphPin* Pin)
{
    const bool HasDecimalPoint = Pin->GetDefaultAsString().Contains(".");
    EConcordValueType Type = Vertex->GetInputInfo()[PinIndex].Type.Get(HasDecimalPoint ? EConcordValueType::Float : EConcordValueType::Int);
    switch (Type)
    {
    case EConcordValueType::Int:
    {
        UConcordTransformerValueInt* TransformerValueInt = NewObject<UConcordTransformerValueInt>(Vertex);
        TransformerValueInt->bIsPinDefaultValue = true;
        LexFromString(TransformerValueInt->Value, *Pin->GetDefaultAsString());
        return TransformerValueInt;
    }
    case EConcordValueType::Float:
    {
        UConcordTransformerValueFloat* TransformerValueFloat = NewObject<UConcordTransformerValueFloat>(Vertex);
        TransformerValueFloat->bIsPinDefaultValue = true;
        LexFromString(TransformerValueFloat->Value, *Pin->GetDefaultAsString());
        return TransformerValueFloat;
    }
    default: checkNoEntry(); return nullptr;
    }
}

UConcordTransformer* FConcordModelGraphSync::GetOwningTransformer(const UEdGraphPin* Pin)
{
    if (UConcordModelGraphBox* BoxNode = Cast<UConcordModelGraphBox>(Pin->GetOwningNode()))
    {
        if (Pin->PinName == "All")
        {
            return BoxNode->GetBox();
        }
        else if (Pin->PinName == "State Set")
        {
            UConcordBoxStateSet* TransformerStateSet = FindObjectFast<UConcordBoxStateSet>(BoxNode->GetBox(), "State Set", true);
            if (!TransformerStateSet) TransformerStateSet = NewObject<UConcordBoxStateSet>(BoxNode->GetBox(), "State Set");
            return TransformerStateSet;
        }
        const int32 FlatBoxLocalIndex = BoxNode->GetPinIndexRV(Pin);
        const FName TransformerElementName { "TransformerElement", FlatBoxLocalIndex };
        UConcordTransformerElement* TransformerElement = FindObjectFast<UConcordTransformerElement>(BoxNode->GetBox(), TransformerElementName, true);
        if (!TransformerElement) TransformerElement = NewObject<UConcordTransformerElement>(BoxNode->GetBox(), TransformerElementName);
        TransformerElement->MultiIndex = ConcordShape::UnflattenIndex(FlatBoxLocalIndex, BoxNode->GetDefaultShape());
        TransformerElement->ConnectedTransformers = { BoxNode->GetBox() };
        return TransformerElement;
    }
    if (UConcordModelGraphParameter* ParameterNode = Cast<UConcordModelGraphParameter>(Pin->GetOwningNode()))
    {
        return ParameterNode->GetParameter();
    }
    if (UConcordModelGraphTransformer* TransformerNode = Cast<UConcordModelGraphTransformer>(Pin->GetOwningNode()))
    {
        return TransformerNode->GetTransformer();
    }
    if (UConcordModelGraphComposite* CompositeNode = Cast<UConcordModelGraphComposite>(Pin->GetOwningNode()))
    {
        return CompositeNode->GetComposite()->GetOutputTransformer(Pin->GetFName());
    }
    if (UConcordModelGraphInstance* InstanceNode = Cast<UConcordModelGraphInstance>(Pin->GetOwningNode()))
    {
        return InstanceNode->GetInstance()->GetOutputTransformer(Pin->GetFName());
    }
    check(false); return nullptr;
}
