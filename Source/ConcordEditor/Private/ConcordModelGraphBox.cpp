// Copyright Jan Klimaschewski. All Rights Reserved.

#include "ConcordModelGraphBox.h"
#include "ConcordBox.h"
#include "SConcordModelGraphPin.h"
#include "Styling/SlateStyleRegistry.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Text/STextBlock.h"
#include "SLevelOfDetailBranchNode.h"
#include "Rendering/DrawElements.h"
#include "Math/Color.h"
#include "Transformers/ConcordTransformerElement.h"

void UConcordModelGraphBox::GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
    if (ContextMenuBuilder.FromPin &&
        ContextMenuBuilder.FromPin->Direction != EGPD_Input) return;
    ContextMenuBuilder.AddAction(MakeShared<FConcordModelGraphAddNodeAction_NewBox>());
}

void UConcordModelGraphBox::SyncModelBox()
{
    GetBox()->Modify();
    GetBox()->DefaultShape = GetDefaultShape();
    GetBox()->StateCount = StateCount;
}

void UConcordModelGraphBox::SyncPins()
{
    const bool bShapePinSet = IsShapePinSet();
    int32 DefaultSize = GetDefaultSize();
    const bool bShouldHaveAllPin = bShapePinSet || DefaultSize > 1;
    if (!bHasAllPin && bShouldHaveAllPin)
    {
        FCreatePinParams CreatePinParams; CreatePinParams.Index = 2;
        CreatePin(EGPD_Output, "Expression", "All", CreatePinParams);
    }
    else if (bHasAllPin && !bShouldHaveAllPin)
    {
        RemovePin(Pins[2]);
    }
    bHasAllPin = bShouldHaveAllPin;

    if (!bHasAllPin || (!bShapePinSet && bEnableIndividualPins))
    {
        while (RVPinsNum() < DefaultSize)
            CreatePin(EGPD_Output, FName("RV"), FName("RVPin", RVPinsNum()));
        while (RVPinsNum() > DefaultSize)
            RemovePin(Pins.Last());
    }
    else
    {
        while (RVPinsNum() > 0)
            RemovePin(Pins.Last());
    }

    OnNodeChanged.Broadcast();
}

TSharedPtr<SGraphNode> UConcordModelGraphBox::CreateVisualWidget()
{
    return SNew(SConcordModelGraphBox, this);
}

void UConcordModelGraphBox::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    UConcordModelGraphNode::PostEditChangeProperty(PropertyChangedEvent);
    if (PropertyChangedEvent.GetPropertyName() == NAME_None) return; // happens when importing from text
    SyncModelBox();
    SyncPins();

    if (!IsShapePinSet() && PropertyChangedEvent.MemberProperty && PropertyChangedEvent.MemberProperty->GetNameCPP() == TEXT("DefaultShape"))
    {
        // fix element transformers on shape change
        for (int32 FlatBoxLocalIndex = 0; FlatBoxLocalIndex < GetDefaultSize(); ++FlatBoxLocalIndex)
        {
            const FName TransformerElementName { "TransformerElement", FlatBoxLocalIndex };
            UConcordTransformerElement* TransformerElement = FindObjectFast<UConcordTransformerElement>(GetBox(), TransformerElementName, true);
            if (TransformerElement) TransformerElement->MultiIndex = ConcordShape::UnflattenIndex(FlatBoxLocalIndex, GetDefaultShape());
        }
    }
}

void UConcordModelGraphBox::AllocateDefaultPins()
{
    UConcordModelGraphConsumer::AllocateDefaultPins();
    CreatePin(EGPD_Output, "Expression", "State Set");
}

void UConcordModelGraphBox::NodeConnectionListChanged()
{
    UConcordModelGraphConsumer::NodeConnectionListChanged();
    SyncPins();
}

void UConcordModelGraphBox::PostPasteNode()
{
    UConcordModelGraphConsumer::PostPasteNode();
    SyncPins();
}

void UConcordModelGraphBox::DestroyNode()
{
    GetModel()->Modify();
    GetModel()->Boxes.Remove(Name);
    GetModel()->Boxes.Shrink();
    UConcordModelGraphNode::DestroyNode();
}

int32 UConcordModelGraphBox::GetPinIndexRV(const UEdGraphPin* Pin)
{
    check(Pin->PinType.PinCategory == "RV");
    return Pin->PinName.GetNumber();
}

int32 UConcordModelGraphBox::GetDefaultSize() const
{
    int32 Size = 1;
    for (const FConcordModelGraphBoxDimension& Dimension : DefaultShape)
        Size *= Dimension.Size;
    return Size;
}

FConcordShape UConcordModelGraphBox::GetDefaultShape() const
{
    FConcordShape OutShape; OutShape.Reserve(DefaultShape.Num());
    for (const FConcordModelGraphBoxDimension& Dimension : DefaultShape)
        OutShape.Add(Dimension.Size);
    return OutShape;
}

int32 UConcordModelGraphBox::RVPinsNum() const
{
    return Pins.Num() - (bHasAllPin ? 3 : 2);
}

bool UConcordModelGraphBox::IsShapePinSet() const
{
    return !Pins[0]->LinkedTo.IsEmpty();
}

void SConcordModelGraphBox::Construct(const FArguments& InArgs, UEdGraphNode* InGraphNode)
{
    GraphNode = InGraphNode;
    bWithDefaultValues = false;
    bWithOutputLabels = true;
    bTitleOnTop = true;
    UConcordModelGraphBox* BoxNode = Cast<UConcordModelGraphBox>(GraphNode);
    BoxNode->OnNodeChanged.AddSP(this, &SConcordModelGraphBox::UpdateGraphNode);
    BoxNode->OnVariationChanged.AddSP(this, &SConcordModelGraphBox::OnVariationChanged);
    BoxNode->OnDistributionsChanged.AddSP(this, &SConcordModelGraphBox::OnDistributionsChanged);
    SetCursor(EMouseCursor::CardinalCross);
    UpdateGraphNode();
}

TSharedRef<SWidget> SConcordModelGraphBox::CreateNodeContentArea()
{
    PinBoxes.Reset();
    return SConcordModelGraphInOutNode::CreateNodeContentArea();
}

void SConcordModelGraphBox::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
    PinToAdd->SetOwner(SharedThis(this));
    if (PinToAdd->GetPinObj()->Direction == EGPD_Input)
    {
        LeftNodeBox->AddSlot()
        .HAlign(HAlign_Left)
        .VAlign(VAlign_Top)
        .Padding(0.0f, 0.0f, 10.0f, 0.0f)
        [ PinToAdd ];
        InputPins.Add(PinToAdd);
        return;
    }
    else OutputPins.Add(PinToAdd);

    if (PinToAdd->GetPinObj()->PinName == "State Set" || PinToAdd->GetPinObj()->PinName == "All")
    {
        RightNodeBox->AddSlot()
        .HAlign(HAlign_Right)
        .VAlign(VAlign_Center)
        .Padding(10.0f, 0.0f, 0.0f, 0.0f)
        [ PinToAdd ];
        return;
    }

    int32 PinIndex = PinToAdd->GetPinObj()->GetFName().GetNumber();
    const UConcordModelGraphBox* BoxNode = Cast<UConcordModelGraphBox>(PinToAdd->GetPinObj()->GetOwningNode());
    FConcordMultiIndex MultiIndex = ConcordShape::UnflattenIndex(PinIndex, BoxNode->GetDefaultShape());
    for (int32 DimIndex = 0; DimIndex < MultiIndex.Num(); ++DimIndex)
        if (ConcordShape::IsRestZero(MultiIndex, DimIndex) && !BoxNode->DefaultShape[DimIndex].Name.IsEmpty())
        {
            AddPinBox(BoxNode->DefaultShape[DimIndex].Name, MultiIndex[DimIndex], DimIndex);
            break;
        }

    if (PinBoxes.IsEmpty()) // add single PinBox without title when no axis names are given
    {
        PinBoxes.Add(SNew(SVerticalBox));
        RightNodeBox->AddSlot()
        .AutoHeight()
        .HAlign(HAlign_Right)
        [ PinBoxes.Last() ];
    }

    PinBoxes.Last()->AddSlot()
    .AutoHeight()
    .HAlign(HAlign_Right)
    [ PinToAdd ];
}

TSharedPtr<SGraphPin> SConcordModelGraphBox::CreatePinWidget(UEdGraphPin* Pin) const
{
    if (Pin->Direction == EGPD_Input || Pin->PinName == "State Set" || Pin->PinName == "All")
        return SConcordModelGraphInOutNode::CreatePinWidget(Pin);
    return SNew(SConcordModelGraphBoxPin, Pin);
}

void SConcordModelGraphBox::AddPinBox(const FString& Name, int32 Index, int32 DimIndex)
{
    FTextBlockStyle TextBlockStyle = FTextBlockStyle::GetDefault();
    TextBlockStyle.SetFontSize(FMath::Max(11 - 2 * DimIndex, 7));
    TextBlockStyle.SetColorAndOpacity(FLinearColor::Gray);

    PinBoxes.Add(SNew(SVerticalBox));
    TSharedRef<SHorizontalBox> NamedPinBox = SNew(SHorizontalBox)
    +SHorizontalBox::Slot()
    .AutoWidth()
    .Padding(4.0f)
    [
        SNew(STextBlock)
        .SimpleTextMode(true)
        .TextStyle(&TextBlockStyle)
        .Text(FText::Format(INVTEXT("{0} {1}"), FText::FromString(Name), Index))
    ]
    +SHorizontalBox::Slot()
    [ PinBoxes.Last() ];
    RightNodeBox->AddSlot()
    .AutoHeight()
    .HAlign(HAlign_Right)
    [ NamedPinBox ];
}

void SConcordModelGraphBox::OnVariationChanged(const TArrayView<const int32>& Variation)
{
    if (HasAllPin() && !Cast<UConcordModelGraphBox>(GraphNode)->bEnableIndividualPins) return;
    const int32 IndexOffset = HasAllPin() ? 2 : 1;
    if (IndexOffset >= OutputPins.Num()) return; // shape override, no rv pins
    for (int32 ValueIndex = 0; ValueIndex < Variation.Num(); ++ValueIndex)
        static_cast<SConcordModelGraphBoxPin*>(&OutputPins[ValueIndex + IndexOffset].Get())->SetValue(Variation[ValueIndex]);
}

void SConcordModelGraphBox::OnDistributionsChanged(const TArrayView<const FConcordDistribution>& Distributions)
{
    if (HasAllPin() && !Cast<UConcordModelGraphBox>(GraphNode)->bEnableIndividualPins) return;
    const int32 IndexOffset = HasAllPin() ? 2 : 1;
    if (IndexOffset >= OutputPins.Num()) return; // shape override, no rv pins
    for (int32 DistributionIndex = 0; DistributionIndex < Distributions.Num(); ++DistributionIndex)
    {
        FLinearColor EntropyColor = UConcordModelGraph::GetEntropyColor(Distributions[DistributionIndex]);
        static_cast<SConcordModelGraphBoxPin*>(&OutputPins[DistributionIndex + IndexOffset].Get())->SetEntropyPinColor(EntropyColor);
    }
}

bool SConcordModelGraphBox::HasAllPin() const
{
    return OutputPins.Num() > 1 && OutputPins[1]->GetPinObj()->PinName == FName("All");
}

void SConcordModelGraphBoxPin::Construct(const FArguments& InArgs, UEdGraphPin* InPin)
{
    SetCursor({ this, &SConcordModelGraphBoxPin::GetPinCursor });
    GraphPinObj = InPin;
    CachedImg_Pin_BackgroundHovered = FSlateStyleRegistry::FindSlateStyle("ConcordEditorStyle")->GetBrush("PinBackgroundHovered");
    Custom_Brush_Disconnected = FSlateStyleRegistry::FindSlateStyle("ConcordEditorStyle")->GetBrush("EntropyPinFrontDisconnected");
    Custom_Brush_Connected = FSlateStyleRegistry::FindSlateStyle("ConcordEditorStyle")->GetBrush("EntropyPinFront");

    SAssignNew(EntropyPinBack, SImage)
    .Image(FSlateStyleRegistry::FindSlateStyle("ConcordEditorStyle")->GetBrush("EntropyPinBack"));
    TSharedRef<SImage> EntropyPinFront = SNew(SImage)
    .Image(this, &SConcordModelGraphBoxPin::GetPinIcon);

    FTextBlockStyle TextBlockStyle = FTextBlockStyle::GetDefault();
    TextBlockStyle.SetFontSize(10);
    TextBlockStyle.SetTypefaceFontName("Bold");
    TextBlockStyle.SetColorAndOpacity(FLinearColor::White);
    SAssignNew(ValueText, STextBlock)
    .TextStyle(&TextBlockStyle)
    .Justification(ETextJustify::Right);

    SAssignNew(PinImage, SHorizontalBox)
    +SHorizontalBox::Slot()
    .AutoWidth()
    .Padding(2.0f, 0.0f)
    .VAlign(VAlign_Center)
    [
        SNew(SBox)
        .WidthOverride(20)
        [
            SNew(SScaleBox)
            .Stretch(EStretch::ScaleToFit)
            .StretchDirection(EStretchDirection::DownOnly)
            [ ValueText.ToSharedRef() ]
        ]
    ]
    +SHorizontalBox::Slot()
    .AutoWidth()
    .VAlign(VAlign_Center)
    [
        SNew(SOverlay)
        +SOverlay::Slot() [ EntropyPinBack.ToSharedRef() ]
        +SOverlay::Slot() [ EntropyPinFront ]
    ];

    SBorder::Construct(SBorder::FArguments()
    .BorderImage(this, &SConcordModelGraphBoxPin::GetPinBorder)
    .OnMouseButtonDown(this, &SConcordModelGraphBoxPin::OnPinNameMouseDown)
    [
        SNew(SLevelOfDetailBranchNode)
        .UseLowDetailSlot(this, &SConcordModelGraphBoxPin::UseLowDetailPinNames)
        .LowDetail() [ EntropyPinFront ]
        .HighDetail() [ PinImage.ToSharedRef() ]
    ]);
}

void SConcordModelGraphBoxPin::SetValue(int32 Value)
{
    ValueText->SetText(FText::Format(INVTEXT("{0}"), Value));
}

void SConcordModelGraphBoxPin::SetEntropyPinColor(const FLinearColor& Color)
{
    EntropyPinBack->SetColorAndOpacity(Color);
}

FConcordModelGraphAddNodeAction_NewBox::FConcordModelGraphAddNodeAction_NewBox()
    : FConcordModelGraphAddNodeAction(FText(),
                                      INVTEXT("Box"),
                                      Cast<UConcordVertex>(UConcordBox::StaticClass()->GetDefaultObject())->GetTooltip(),
                                      INVTEXT("Add New Concord Box"))
{}

UEdGraphNode* FConcordModelGraphAddNodeAction_NewBox::MakeNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin)
{
    FGraphNodeCreator<UConcordModelGraphBox> NodeCreator(*ParentGraph);
    UConcordModelGraphBox* NewGraphBox = NodeCreator.CreateUserInvokedNode();
    NewGraphBox->Name = FName("NewBox");
    NewGraphBox->DefaultShape.AddDefaulted();
    NewGraphBox->StateCount = 2;

    NewGraphBox->Vertex = NewObject<UConcordBox>(NewGraphBox->GetModel(), NAME_None, RF_Transactional);
    NewGraphBox->SyncModelName(false);
    NewGraphBox->SyncModelBox();
    NodeCreator.Finalize();
    NewGraphBox->SyncPins();

    return NewGraphBox;
}
