// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "SConcordModelGraphInOutNode.h"
#include "ConcordModelGraph.h"
#include "ConcordModelGraphModelNode.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Styling/SlateStyleRegistry.h"

void SConcordModelGraphInOutNode::Construct(const FArguments& InArgs, UEdGraphNode* InGraphNode)
{
    GraphNode = InGraphNode;
    bWithDefaultValues = InArgs._bWithDefaultValues;
    bWithOutputLabels = InArgs._bWithOutputLabels;
    bTitleOnTop = InArgs._bTitleOnTop;
    UConcordModelGraphNode* ConcordNode = Cast<UConcordModelGraphNode>(GraphNode);
    ConcordNode->OnNodeChanged.AddSP(this, &SConcordModelGraphInOutNode::UpdateGraphNode);
    SetCursor(EMouseCursor::CardinalCross);
    UpdateGraphNode();
}

TSharedRef<SWidget> SConcordModelGraphInOutNode::CreateNodeContentArea()
{
    if (bTitleOnTop)
    {
        TSharedRef<SBorder> Border = StaticCastSharedRef<SBorder>(SConcordModelGraphNode::CreateNodeContentArea());
        Border->SetPadding(FMargin(12.0f, 3.0f));
        return Border;
    }

    TSharedPtr<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode);
    return SNew(SBorder)
    .BorderImage(FAppStyle::GetBrush("NoBorder"))
    .HAlign(HAlign_Fill)
    .VAlign(VAlign_Fill)
    .Padding(FMargin(12.0f, 3.0f))
    [
        SNew(SHorizontalBox)
        +SHorizontalBox::Slot()
        .AutoWidth()
        [ SAssignNew(LeftNodeBox, SVerticalBox) ]
        +SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        .Padding(FMargin(15.0f, 4.0f))
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            [ CreateTitleWidget(NodeTitle) ]
            + SVerticalBox::Slot()
            .AutoHeight()
            [ NodeTitle.ToSharedRef() ]
        ]
        +SHorizontalBox::Slot()
        .AutoWidth()
        [ SAssignNew(RightNodeBox, SVerticalBox) ]
    ];
}

TSharedRef<SWidget> SConcordModelGraphInOutNode::CreateTitleWidget(TSharedPtr<SNodeTitle> NodeTitle)
{
    if (bTitleOnTop) return SConcordModelGraphNode::CreateTitleWidget(NodeTitle);

    const FName StyleName = GraphNode->GetNodeTitle(ENodeTitleType::FullTitle).ToString().Len() == 1 ? "InOutNodeTitleSymbol" : "InOutNodeTitle";
    SAssignNew(InlineEditableText, SInlineEditableTextBlock)
    .Style(&FSlateStyleRegistry::FindSlateStyle("ConcordEditorStyle")->GetWidgetStyle<FInlineEditableTextBlockStyle>(StyleName))
    .Text(NodeTitle.Get(), &SNodeTitle::GetHeadTitle)
    .Justification(ETextJustify::Center)
    .OnVerifyTextChanged(this, &SConcordModelGraphInOutNode::OnVerifyNameTextChanged)
    .OnTextCommitted(this, &SConcordModelGraphInOutNode::OnNameTextCommited)
    .IsReadOnly(this, &SConcordModelGraphInOutNode::IsNameReadOnly)
    .IsSelected(this, &SConcordModelGraphInOutNode::IsSelectedExclusively);

    return InlineEditableText.ToSharedRef();
}

void SConcordModelGraphInOutNode::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
    PinToAdd->SetOwner(SharedThis(this));
    if (PinToAdd->GetPinObj()->Direction == EGPD_Input)
    {
        LeftNodeBox->AddSlot()
        .HAlign(HAlign_Left)
        .VAlign(VAlign_Center)
        .Padding(0.0f, 0.0f, bTitleOnTop ? 10.0f : 0.0f, 0.0f)
        [ PinToAdd ];
        InputPins.Add(PinToAdd);
    }
    else
    {
        RightNodeBox->AddSlot()
        .HAlign(HAlign_Right)
        .VAlign(VAlign_Center)
        .Padding(bTitleOnTop ? 10.0f : 0.0f, 0.0f, 0.0f, 0.0f)
        [ PinToAdd ];
        OutputPins.Add(PinToAdd);
    }
}

TSharedPtr<SGraphPin> SConcordModelGraphInOutNode::CreatePinWidget(UEdGraphPin* Pin) const
{
    return SNew(SConcordModelGraphPin, Pin)
    .WithDefaultValue(bWithDefaultValues)
    .WithLabel(Pin->Direction == EGPD_Input || bWithOutputLabels);
}

void SConcordModelGraphInOutNode::CreateBelowPinControls(TSharedPtr<SVerticalBox> MainBox) 
{
    if (bTitleOnTop) { SConcordModelGraphNode::CreateBelowPinControls(MainBox); return; }

    static_cast<TPanelChildren<SVerticalBox::FSlot>*>(MainBox->GetChildren())->RemoveAt(0); // remove title widget
}

const FSlateBrush* SConcordModelGraphInOutNode::GetNodeBodyBrush() const
{
    UConcordModelGraphInstance* Instance = Cast<UConcordModelGraphInstance>(GraphNode);
    if (!Instance) return SConcordModelGraphNode::GetNodeBodyBrush();
    return FSlateStyleRegistry::FindSlateStyle("ConcordEditorStyle")->GetBrush("ObservedInstanceNodeBody");
}
