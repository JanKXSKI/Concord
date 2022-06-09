// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "SConcordModelGraphSink.h"
#include "Styling/SlateStyleRegistry.h"
#include "Widgets/Images/SImage.h"
#include "SLevelOfDetailBranchNode.h"
#include "Math/Color.h"
#include "Rendering/SlateRenderTransform.h"
#include "SConcordModelGraphPin.h"

void SConcordModelGraphSink::Construct(const FArguments& InArgs, UEdGraphNode* InGraphNode)
{
    GraphNode = InGraphNode;
    bWithPinLabels = InArgs._WithPinLabels;
    bWithPinDefaultValues = InArgs._WithPinDefaultValues;
    AddSinkDelegates();
    SetCursor(EMouseCursor::CardinalCross);
    UpdateGraphNode();
}

TSharedRef<SWidget> SConcordModelGraphSink::CreateNodeContentArea()
{
    return SNew(SBorder)
    .BorderImage(FEditorStyle::GetBrush("NoBorder"))
    .HAlign(HAlign_Fill)
    .VAlign(VAlign_Fill)
    .Padding(FMargin(12.0f, 3.0f))
    [ 
        SAssignNew(ContentBox, SHorizontalBox)
        +SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        [ SAssignNew(LeftNodeBox, SVerticalBox) ]
    ];
}

void SConcordModelGraphSink::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
    PinToAdd->SetOwner(SharedThis(this));
    LeftNodeBox->AddSlot()
    .HAlign(HAlign_Left)
    .VAlign(VAlign_Center)
    [ PinToAdd ];
    InputPins.Add(PinToAdd);
}

TSharedPtr<SGraphPin> SConcordModelGraphSink::CreatePinWidget(UEdGraphPin* Pin) const
{
    return SNew(SConcordModelGraphPin, Pin)
           .WithLabel(bWithPinLabels)
           .WithDefaultValue(bWithPinDefaultValues);
}

void SConcordModelGraphSink::CreateBelowPinControls(TSharedPtr<SVerticalBox> MainBox) 
{
    ContentBox->AddSlot()
    [ CreateAdditionalSinkContent() ];
}
