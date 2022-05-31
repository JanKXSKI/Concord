// Copyright Jan Klimaschewski. All Rights Reserved.

#include "ConcordModelGraph.h"
#include "ConcordVertex.h"
#include "Transformers/ConcordTransformerGet.h"
#include "ConcordModelGraphBox.h"
#include "ConcordModelGraphFactor.h"
#include "ConcordModelGraphOutput.h"
#include "ConcordModelGraphTransformer.h"
#include "ConcordModelGraphParameter.h"
#include "ConcordModelGraphModelNode.h"
#include "ConcordModelNativization.h"
#include "ScopedTransaction.h"
#include "EdGraph/EdGraph.h"
#include "Factories.h"
#include "FileHelpers.h"

void UConcordModelGraph::Nativize()
{
    if (!NativeModel) { Modify(); NativeModel = FConcordModelNativization::CreateNativeModel(GetModel()); }
    TOptional<FConcordError> Error = FConcordModelNativization::SetupNativeModel(NativeModel, GetModel(), NativizationCycleMode, bLiveCoding);
    OnNativizeOptionalError.Broadcast(Error);
}

UConcordModel* UConcordModelGraph::GetModel() const
{
    return Cast<UConcordModel>(GetOuter());
}

void UConcordModelGraph::SetupShapesAndTypes()
{
    TSet<UConcordVertex*> VisitedVertices;
    SetupShapesAndTypes(VisitedVertices);
}

void UConcordModelGraph::SetupShapesAndTypes(TSet<UConcordVertex*>& VisitedVertices)
{
    for (UEdGraphNode* Node : Nodes)
        if (UConcordModelGraphConsumer* ConsumerNode = Cast<UConcordModelGraphConsumer>(Node))
            if (!ConsumerNode->IsA<UConcordTransformer>())
                ConsumerNode->Vertex->SetupGraph(VisitedVertices);
}

FLinearColor UConcordModelGraph::GetEntropyColor(const FConcordDistribution& Distribution)
{
    float Entropy = 0.0f;
    for (float Prob : Distribution) if (Prob > 0.0f)
        Entropy -= Prob * FMath::LogX(Distribution.Num(), Prob);
    float Alpha = 1.0f - Entropy;
    Alpha = (Alpha + Alpha * Alpha) * 0.5f;
    FLinearColor Color = FLinearColor::MakeFromColorTemperature(FMath::Lerp(1000.0f, 10000.0f, Alpha));
    Color = Color.LinearRGBToHSV();
    Color.G += FMath::GetMappedRangeValueClamped(TRange<float>(0.2f, 0.8f), TRange<float>(0.0f, 0.6f), Alpha);
    Color.B += FMath::GetMappedRangeValueClamped(TRange<float>(0.6f, 0.8f), TRange<float>(0.0f, 1.0f), Alpha);
    return Color.HSVToLinearRGB();
}

TArray<UEdGraphPin*> UConcordModelGraph::GetSelectedPins(EEdGraphPinDirection Direction) const
{
    TArray<UConcordModelGraphNode*> SortedSelectedNodes = SelectedNodes;
    SortedSelectedNodes.Sort([](const auto& A, const auto& B){ return A.NodePosY < B.NodePosY; });
    TArray<UEdGraphPin*> Pins;
    for (UConcordModelGraphNode* Node : SortedSelectedNodes)
        for (UEdGraphPin* Pin : Node->Pins)
            if (Pin->Direction == Direction)
                Pins.Add(Pin);
    return MoveTemp(Pins);
}

const FPinConnectionResponse UConcordModelGraphSchema::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const
{
    if (A->Direction == B->Direction)
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Cannot connect output to output or input to input."));

    if (A->PinType.PinCategory == "Emission" && B->PinName != "All" ||
        A->PinName != "All" && B->PinType.PinCategory == "Emission")
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Emission inputs can only be connected to All outputs."));

    if (A->PinType.PinCategory == "Emission" && B->PinName == "All")
        return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_A, TEXT("Make emission connection."));

    if (A->PinName == "All" && B->PinType.PinCategory == "Emission")
        return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_B, TEXT("Make emission connection."));

    if (A->PinType.PinCategory == "RV" && B->PinType.PinCategory == "Viewer")
        return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_B, TEXT("Make Random Variable to Viewer connection."));

    if (A->PinType.PinCategory == "Viewer" && B->PinType.PinCategory == "RV")
        return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_A, TEXT("Make Random Variable to Viewer connection."));

    if (A->PinType.PinCategory == "RV" && B->PinType.PinCategory == "Expression" ||
        A->PinType.PinCategory == "Expression" && B->PinType.PinCategory == "RV")
        return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT("Make Random Variable to Index connection."));

    if (A->PinType.PinCategory == "Expression" && B->PinType.PinCategory == "Expression")
    {
        if ((A->Direction == EGPD_Output && ConnectionWouldCauseCycle(A, B)) ||
            (B->Direction == EGPD_Output && ConnectionWouldCauseCycle(B, A)))
            return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Connection would cause a cycle."));
        return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT("Make Index to Index connection."));
    }

    return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Not implemented by this schema."));
}

void UConcordModelGraphSchema::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotifcation) const
{
    const FScopedTransaction Transaction(INVTEXT("Break Pin Links"));
    UEdGraphSchema::BreakPinLinks(TargetPin, bSendsNodeNotifcation);
}

void UConcordModelGraphSchema::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const
{
    const FScopedTransaction Transaction(INVTEXT("Break Pin Link"));
    UEdGraphSchema::BreakSinglePinLink(SourcePin, TargetPin);
}

bool UConcordModelGraphSchema::ConnectionWouldCauseCycle(const UEdGraphPin* OutputPin, const UEdGraphPin* CycleStartInputPin) const
{
    for (const UEdGraphPin* Pin : OutputPin->GetOwningNode()->Pins)
    {
        if (Pin->Direction == EGPD_Output) continue;
        if (Pin == CycleStartInputPin) return true;
        for (const UEdGraphPin* ConnectedOutputPin : Pin->LinkedTo)
            if (ConnectionWouldCauseCycle(ConnectedOutputPin, CycleStartInputPin))
                return true;
    }
    return false;
}

void UConcordModelGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
    UEdGraphSchema::GetGraphContextActions(ContextMenuBuilder);

    if (!ContextMenuBuilder.FromPin)
    {
        const TArray<UEdGraphPin*> TargetPins = Cast<UConcordModelGraph>(ContextMenuBuilder.CurrentGraph)->GetSelectedPins(EGPD_Input);
        if (!TargetPins.IsEmpty()) ContextMenuBuilder.AddAction(MakeShared<FConcordModelGraphFromParameterAction>(TargetPins));

        const TArray<UEdGraphPin*> SourcePins = Cast<UConcordModelGraph>(ContextMenuBuilder.CurrentGraph)->GetSelectedPins(EGPD_Output);
        if (!SourcePins.IsEmpty()) ContextMenuBuilder.AddAction(MakeShared<FConcordModelGraphToOutputAction>(SourcePins));
    }

    if (ContextMenuBuilder.FromPin && ContextMenuBuilder.FromPin->Direction == EGPD_Output)
    {
        TArray<UEdGraphPin*> TargetPins = Cast<UConcordModelGraph>(ContextMenuBuilder.CurrentGraph)->GetSelectedPins(EGPD_Input);
        ContextMenuBuilder.AddAction(MakeShared<FConcordModelGraphExplodeAction>(TargetPins));
    }
}

bool UConcordModelGraphNode::CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const
{
    return Schema->IsA(UConcordModelGraphSchema::StaticClass());
}

UConcordModel* UConcordModelGraphNode::GetModel() const
{
    return Cast<UConcordModelGraph>(GetGraph())->GetModel();
}

UConcordModelGraph* UConcordModelGraphNode::GetGraph() const
{
    return Cast<UConcordModelGraph>(UEdGraphNode::GetGraph());
}

FText UConcordModelGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    return FText::FromName(Name);
}

void UConcordModelGraphNode::OnRenameNode(const FString& NewNameString)
{
    NameToRemove = Name;
    Name = FName(NewNameString);
    OnNameChanged();
}

void UConcordModelGraphNode::OnNameChanged()
{
    check(NameToRemove != FName());
    Name = GetModel()->SanitizeName(Name);
    if (Name != NameToRemove) SyncModelName(true);
    NameToRemove = FName();
}

void UConcordModelGraphNode::SyncModelName(bool bRemoveOldName)
{
    FString NameString;
    while (GetModel()->IsNameTaken(Name))
    {
        NameString = Name.ToString();
        int32 CheckIndex = NameString.Len() - 1;
        while (CheckIndex >= 0 && NameString[CheckIndex] == TEXT('9')) NameString[CheckIndex--] = TEXT('0');
        if (CheckIndex >= 0 && NameString[CheckIndex] >= TEXT('0') && NameString[CheckIndex] < TEXT('9'))
        {
            ++NameString[CheckIndex];
            Name = FName(NameString);
        }
        else Name.SetNumber(Name.GetNumber() + 1);
    }
    GetModel()->Modify();
    if (UConcordModelGraphBox* BoxNode = Cast<UConcordModelGraphBox>(this))
    {
        if (bRemoveOldName) GetModel()->Boxes.Remove(NameToRemove);
        GetModel()->Boxes.Add(Name, BoxNode->GetBox());
    }
    else if (UConcordModelGraphParameter* ParameterNode = Cast<UConcordModelGraphParameter>(this))
    {
        if (bRemoveOldName)
        {
            GetModel()->Parameters.Remove(NameToRemove);
            GetModel()->OrderedParameterNames.RemoveSingle(NameToRemove);
        }
        GetModel()->Parameters.Add(Name, ParameterNode->GetParameter());
        GetModel()->OrderedParameterNames.Add(Name);
    }
    else if (UConcordModelGraphOutput* OutputNode = Cast<UConcordModelGraphOutput>(this))
    {
        if (bRemoveOldName)
        {
            GetModel()->Outputs.Remove(NameToRemove);
            GetModel()->OrderedOutputNames.RemoveSingle(NameToRemove);
        }
        GetModel()->Outputs.Add(Name, OutputNode->GetOutput());
        GetModel()->OrderedOutputNames.Add(Name);
    }
    else if (UConcordModelGraphComposite* CompositeNode = Cast<UConcordModelGraphComposite>(this))
    {
        if (bRemoveOldName) GetModel()->Composites.Remove(NameToRemove);
        GetModel()->Composites.Add(Name, CompositeNode->GetComposite());
    }
    else if (UConcordModelGraphInstance* InstanceNode = Cast<UConcordModelGraphInstance>(this))
    {
        if (bRemoveOldName) GetModel()->Instances.Remove(NameToRemove);
        GetModel()->Instances.Add(Name, InstanceNode->GetInstance());
    }
}

struct FVertexObjectTextFactory : public FCustomizableTextObjectFactory
{
public:
    using FCustomizableTextObjectFactory::FCustomizableTextObjectFactory;
    bool CanCreateClass(UClass* ObjectClass, bool& bOmitSubObjs) const override
    {
        return ObjectClass->IsChildOf(UConcordVertex::StaticClass());
    }
    void ProcessConstructedObject(UObject* CreatedObject) override
    {
        CreatedVertex = Cast<UConcordVertex>(CreatedObject);
    }
    UConcordVertex* CreatedVertex = nullptr;
};

void UConcordModelGraphNode::PostEditImport()
{
    // Copying from one model graph to another might assign the wrong Vertex by unqualified name, always rebuild.
    // Rebuilding might put nullptrs or references to nodes from other graphs in the ConnectedTransformers array,
    // but those will be overwritten by the next sync in PostPasteNode.
    FVertexObjectTextFactory VertexObjectTextFactory(GWarn);
    VertexObjectTextFactory.ProcessBuffer(GetModel(), RF_Transactional, SerializedVertex);
    Vertex = VertexObjectTextFactory.CreatedVertex;
    if (!Vertex)
    {
        Vertex = NewObject<UConcordMissingVertex>(GetModel(), NAME_None, RF_Transactional);
        for (UEdGraphPin* Pin : Pins) Pin->bWasTrashed = true;
        Pins.Empty();
    }
    SyncModelName(false);
}

void UConcordModelGraphNode::PreEditChange(FProperty* PropertyAboutToChange)
{
    UEdGraphNode::PreEditChange(PropertyAboutToChange);
    if (PropertyAboutToChange && PropertyAboutToChange->GetFName() == "Name") NameToRemove = Name;
}

void UConcordModelGraphNode::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    UEdGraphNode::PostEditChangeProperty(PropertyChangedEvent);
    if (PropertyChangedEvent.GetPropertyName() == "Name") OnNameChanged();
}

FText UConcordModelGraphNode::GetTooltipText() const
{
    if (!Vertex) return INVTEXT("Mising tooltip.");
    return Vertex->GetTooltip();
}

TOptional<FConcordError> UConcordModelGraphNode::SetupShapeAndType()
{
    TSet<UConcordVertex*> VisitedVertices;
    return SetupShapeAndType(VisitedVertices);
}

TOptional<FConcordError> UConcordModelGraphNode::SetupShapeAndType(TSet<UConcordVertex*>& VisitedVertices)
{
    return Vertex->SetupGraph(VisitedVertices);
}

void UConcordModelGraphNode::OnInputOutputInterfaceChanged()
{
    if (!GetModel()->InputOutputInterfaceListeners.IsEmpty())
    {
        GetModel()->OnInputOutputInterfaceChanged();
        SavePackage(GetPackage());
    }
}

void UConcordModelGraphNode::SavePackage(UPackage* Package)
{
    const bool PrevSilent = GIsSilent;
    GIsSilent = true;
    const auto SaveResult = FEditorFileUtils::PromptForCheckoutAndSave({ Package }, true, false, nullptr, true, false);
    GIsSilent = PrevSilent;
    check(SaveResult == FEditorFileUtils::EPromptReturnCode::PR_Success);
}

FConcordModelGraphAddNodeAction::FConcordModelGraphAddNodeAction(FText InNodeCategory, FText InMenuDesc, FText InToolTip, FText InTransactionText, FVector2D InLocationOffset)
    : FEdGraphSchemaAction(InNodeCategory, InMenuDesc, InToolTip, 0)
    , TransactionText(InTransactionText)
    , LocationOffset(InLocationOffset)
{}

UEdGraphNode* FConcordModelGraphAddNodeAction::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
    const FScopedTransaction Transaction(TransactionText);
    ParentGraph->Modify();
    Cast<UConcordModelGraph>(ParentGraph)->GetModel()->Modify();
    UEdGraphNode* NewNode = MakeNode(ParentGraph, FromPin);
    NewNode->NodePosX = Location.X + LocationOffset.X;
    NewNode->NodePosY = Location.Y + LocationOffset.Y;
    if (FromPin) for (UEdGraphPin* Pin : NewNode->Pins)
    {
        if (FromPin->Direction == Pin->Direction) continue;
        ParentGraph->GetSchema()->TryCreateConnection(Pin, FromPin);
        NewNode->NodeConnectionListChanged();
        FromPin->GetOwningNode()->NodeConnectionListChanged();
        break;
    }
    return NewNode;
}

FConcordModelGraphFromParameterAction::FConcordModelGraphFromParameterAction(const TArray<UEdGraphPin*>& InTargetPins)
    : FEdGraphSchemaAction(INVTEXT("Meta"), INVTEXT("From Parameter"), INVTEXT("Connects parameters to the selected input pins, creating them when needed."), 0)
    , TargetPins(InTargetPins)
{}

UEdGraphNode* FConcordModelGraphFromParameterAction::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
    const FScopedTransaction Transaction(INVTEXT("From Parameter"));
    ParentGraph->Modify();
    Cast<UConcordModelGraph>(ParentGraph)->GetModel()->Modify();
    const double OffsetY = 70.0;
    TArray<UConcordModelGraphParameter*> GraphParameters;
    ParentGraph->GetNodesOfClass(GraphParameters);
    for (int32 PinIndex = 0; PinIndex < TargetPins.Num(); ++PinIndex)
    {
        UEdGraphPin* TargetPin = TargetPins[PinIndex];
        if (!TargetPin->LinkedTo.IsEmpty())
        {
            TargetPins.RemoveAt(PinIndex--);
            continue;
        }
        UConcordModelGraphParameter** FoundGraphParameter = GraphParameters.FindByPredicate([TargetPin](const auto& GraphParameter){ return GraphParameter->Name == TargetPin->PinName; });
        if (FoundGraphParameter)
        {
            ParentGraph->GetSchema()->TryCreateConnection((*FoundGraphParameter)->GetPinWithDirectionAt(0, EGPD_Output), TargetPin);
            (*FoundGraphParameter)->NodeConnectionListChanged();
            TargetPin->GetOwningNode()->NodeConnectionListChanged();
            TargetPins.RemoveAt(PinIndex--);
            continue;
        }
        FGraphNodeCreator<UConcordModelGraphParameter> NodeCreator(*ParentGraph);
        UConcordModelGraphParameter* Node = NodeCreator.CreateUserInvokedNode(false);
        Node->Name = TargetPin->PinName;
        const int32 NodeLocalPinIndex = TargetPin->GetOwningNode()->GetPinIndex(TargetPin);
        const UConcordModelGraphNode* TargetNode = Cast<UConcordModelGraphNode>(TargetPin->GetOwningNode());
        const UConcordVertex::FInputInfo InputInfo = TargetNode->Vertex->GetInputInfo()[NodeLocalPinIndex];
        const EConcordValueType Type = InputInfo.Type.Get(EConcordValueType::Int);
        switch (Type)
        {
        case EConcordValueType::Int: Node->Vertex = NewObject<UConcordParameterInt>(Node->GetModel(), NAME_None, RF_Transactional); break;
        case EConcordValueType::Float: Node->Vertex = NewObject<UConcordParameterFloat>(Node->GetModel(), NAME_None, RF_Transactional); break;
        default: checkNoEntry();
        }
        if (auto CompositeNode = Cast<UConcordModelGraphComposite>(TargetNode))
            Node->GetParameter()->DefaultShape = CompositeNode->Model->Parameters[TargetPin->PinName]->DefaultShape;
        else
            Node->GetParameter()->DefaultShape = InputInfo.Shape.Get({ 1 });
        Node->SyncModelName(false);
        NodeCreator.Finalize();
        Node->NodePosX = Location.X;
        Node->NodePosY = Location.Y + PinIndex * OffsetY;
        ParentGraph->GetSchema()->TryCreateConnection(Node->GetPinWithDirectionAt(0, EGPD_Output), TargetPin);
        Node->NodeConnectionListChanged();
        TargetPin->GetOwningNode()->NodeConnectionListChanged();
    }
    return nullptr;
}

FConcordModelGraphToOutputAction::FConcordModelGraphToOutputAction(const TArray<UEdGraphPin*>& InSourcePins)
    : FEdGraphSchemaAction(INVTEXT("Meta"), INVTEXT("To Output"), INVTEXT("Creates a Concord output for each selected output pin."), 0)
    , SourcePins(InSourcePins)
{}

UEdGraphNode* FConcordModelGraphToOutputAction::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
    const FScopedTransaction Transaction(INVTEXT("To Output"));
    ParentGraph->Modify();
    Cast<UConcordModelGraph>(ParentGraph)->GetModel()->Modify();
    const double OffsetY = 70.0;
    for (int32 PinIndex = 0; PinIndex < SourcePins.Num(); ++PinIndex)
    {
        FGraphNodeCreator<UConcordModelGraphOutput> NodeCreator(*ParentGraph);
        UConcordModelGraphOutput* Node = NodeCreator.CreateUserInvokedNode(false);
        Node->Name = SourcePins[PinIndex]->GetFName();
        Node->Vertex = NewObject<UConcordOutput>(Node->GetModel(), NAME_None, RF_Transactional);
        Node->SyncModelName(false);
        Node->bDisplayOutput = false;
        NodeCreator.Finalize();
        Node->NodePosX = Location.X;
        Node->NodePosY = Location.Y + PinIndex * OffsetY;
        ParentGraph->GetSchema()->TryCreateConnection(SourcePins[PinIndex], Node->GetPinWithDirectionAt(0, EGPD_Input));
        SourcePins[PinIndex]->GetOwningNode()->NodeConnectionListChanged();
        Node->NodeConnectionListChanged();
    }
    return nullptr;
}

FConcordModelGraphExplodeAction::FConcordModelGraphExplodeAction(const TArray<UEdGraphPin*>& InTargetPins)
    : FEdGraphSchemaAction(INVTEXT("Meta"), INVTEXT("Explode"), INVTEXT("Explodes the first dimension of the output pin that was dragged from into the selected input pins."), 0)
    , TargetPins(InTargetPins)
{}

UEdGraphNode* FConcordModelGraphExplodeAction::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
    const FScopedTransaction Transaction(INVTEXT("Explode"));
    ParentGraph->Modify();
    Cast<UConcordModelGraph>(ParentGraph)->GetModel()->Modify();
    const double OffsetY = 35.0;
    const int32 PinCount = TargetPins.IsEmpty() ? Cast<UConcordModelGraphNode>(FromPin->GetOwningNode())->Vertex->GetShape()[0] : TargetPins.Num();
    for (int32 PinIndex = 0; PinIndex < PinCount; ++PinIndex)
    {
        FGraphNodeCreator<UConcordModelGraphTransformer> NodeCreator(*ParentGraph);
        UConcordModelGraphTransformer* Node = NodeCreator.CreateUserInvokedNode(false);
        UConcordTransformerGet* Transformer = NewObject<UConcordTransformerGet>(Node->GetModel(), NAME_None, RF_Transactional);
        Transformer->Index = PinIndex;
        Node->Vertex = Transformer;
        NodeCreator.Finalize();
        Node->NodePosX = Location.X;
        Node->NodePosY = Location.Y + PinIndex * OffsetY;
        ParentGraph->GetSchema()->TryCreateConnection(FromPin, Node->GetPinWithDirectionAt(0, EGPD_Input));
        if (!TargetPins.IsEmpty()) ParentGraph->GetSchema()->TryCreateConnection(Node->GetPinWithDirectionAt(0, EGPD_Output), TargetPins[PinIndex]);
        Node->NodeConnectionListChanged();
        if (!TargetPins.IsEmpty()) TargetPins[PinIndex]->GetOwningNode()->NodeConnectionListChanged();
    }
    FromPin->GetOwningNode()->NodeConnectionListChanged();
    return nullptr;
}
