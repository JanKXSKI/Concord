// Copyright Jan Klimaschewski. All Rights Reserved.

#include "SConcordModelGraphPin.h"
#include "ConcordModelGraph.h"
#include "ConcordModelGraphModelNode.h"
#include "ConcordVertex.h"
#include "ConcordTransformer.h"
#include "Styling/SlateStyleRegistry.h"
#include "Widgets/Images/SImage.h"
#include "SLevelOfDetailBranchNode.h"
#include "Math/Color.h"
#include "Rendering/SlateRenderTransform.h"

void SConcordModelGraphPin::Construct(const FArguments& InArgs, UEdGraphPin* InPin)
{
    SetCursor({ this, &SConcordModelGraphPin::GetPinCursor });
    GraphPinObj = InPin;
    CachedImg_Pin_BackgroundHovered = FSlateStyleRegistry::FindSlateStyle("ConcordEditorStyle")->GetBrush("PinBackgroundHovered");

    SAssignNew(PinImage, SImage)
    .Image(this, &SConcordModelGraphPin::GetPinIcon)
    .ColorAndOpacity(this, &SConcordModelGraphPin::GetColor);

    TSharedPtr<SHorizontalBox> PinWidget;
    if (InPin->Direction == EGPD_Input)
    {
        PinWidget = SNew(SHorizontalBox)
        +SHorizontalBox::Slot()
        .AutoWidth()
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Center)
        .Padding(4.0f, 5.0f)
        [ PinImage.ToSharedRef() ];
        if (InArgs._WithLabel && !GetPinLabel().IsEmpty())
            PinWidget->AddSlot()
            .Padding(4.0f, 0.0f)
            .VAlign(VAlign_Center)
            [ GetLabelWidget("Graph.Node.PinName") ];
        if (InArgs._WithDefaultValue)
            PinWidget->AddSlot()
            .Padding(4.0f, 0.0f)
            .AutoWidth()
            .VAlign(VAlign_Center)
            [ GetDefaultValueWidget() ];
    }
    else
    {
        PinWidget = SNew(SHorizontalBox);
        if (InArgs._WithLabel)
            PinWidget->AddSlot()
            .Padding(4.0f, 0.0f)
            .VAlign(VAlign_Center)
            [ GetLabelWidget("Graph.Node.PinName") ];
        PinWidget->AddSlot()
        .AutoWidth()
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Center)
        .Padding(4.0f, 5.0f)
        [ PinImage.ToSharedRef() ];
    }

    SBorder::Construct(SBorder::FArguments()
                       .BorderImage(this, &SConcordModelGraphPin::GetPinBorder)
                       .BorderBackgroundColor(this, &SConcordModelGraphPin::GetPinColor)
                       .OnMouseButtonDown(this, &SConcordModelGraphPin::OnPinNameMouseDown)
                       [
                           SNew(SLevelOfDetailBranchNode)
                           .UseLowDetailSlot(this, &SConcordModelGraphPin::UseLowDetailPinNames)
                           .LowDetail() [ PinImage.ToSharedRef() ]
                           .HighDetail() [ PinWidget.ToSharedRef() ]
                       ]);
}

TSharedRef<SWidget>	SConcordModelGraphPin::GetDefaultValueWidget()
{
    TSharedPtr<TDefaultNumericTypeInterface<double>> TypeInterface = MakeShared<TDefaultNumericTypeInterface<double>>();
    TypeInterface->SetMinFractionalDigits(0);

    return SNew(SBox)
    .MinDesiredWidth(18)
    .MaxDesiredWidth(400)
    [
        SNew(SNumericEntryBox<double>)
        .EditableTextBoxStyle(FEditorStyle::Get(), "Graph.EditableTextBox")
        .BorderForegroundColor(FSlateColor::UseForeground())
        .Visibility(this, &SConcordModelGraphPin::GetDefaultValueVisibility)
        .IsEnabled(this, &SConcordModelGraphPin::GetDefaultValueIsEditable)
        .Value(this, &SConcordModelGraphPin::GetNumericValue)
        .OnValueCommitted(this, &SConcordModelGraphPin::SetNumericValue)
        .TypeInterface(TypeInterface)
    ];
}

TSharedRef<SWidget> SConcordModelGraphPin::GetLabelWidget(const FName& InPinLabelStyle)
{
    return SNew(STextBlock)
    .Text(GetPinLabel())
    .TextStyle(FEditorStyle::Get(), InPinLabelStyle)
    .Visibility(this, &SConcordModelGraphPin::GetPinLabelVisibility)
    .ColorAndOpacity(this, &SConcordModelGraphPin::GetPinTextColor)
    .Justification(ETextJustify::Center);
}

TOptional<double> SConcordModelGraphPin::GetNumericValue() const
{
    double Num = 0.0;
    LexFromString(Num, *GraphPinObj->GetDefaultAsString());
    return Num;
}

void SConcordModelGraphPin::SetNumericValue(double InValue, ETextCommit::Type CommitType)
{
    if(GraphPinObj->IsPendingKill()) return;
    const FString TypeValueString = ShouldLexInt(InValue) ? LexToString(int32(InValue)) : LexToString(InValue);
    if (GraphPinObj->GetDefaultAsString() != TypeValueString)
    {
        const FScopedTransaction Transaction(NSLOCTEXT("GraphEditor", "ChangeNumberPinValue", "Change Number Pin Value"));
        GraphPinObj->Modify();
        GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, *TypeValueString);
    }
}

bool SConcordModelGraphPin::ShouldLexInt(double InValue) const
{
    if (TOptional<EConcordValueType> Type = GetType())
        return GetType().GetValue() == EConcordValueType::Int;
    else
        return InValue == int32(InValue);
}

TOptional<EConcordValueType> SConcordModelGraphPin::GetType() const
{
    const UConcordModelGraphNode* Node = Cast<UConcordModelGraphNode>(GraphPinObj->GetOwningNode());
    const UConcordVertex* Vertex = Node->Vertex;
    if (GraphPinObj->Direction == EGPD_Input)
    {
        if (!Vertex) return EConcordValueType::Int;
        const int32 Index = Node->Pins.Find(GraphPinObj);
        check(Index < Vertex->GetInputInfo().Num());
        if (TOptional<EConcordValueType> Type = Vertex->GetInputInfo()[Index].Type) return Type.GetValue();
        if (!GraphPinObj->LinkedTo.IsEmpty()) return Vertex->GetConnectedTransformers()[Index]->GetType();
        return {};
    }
    else
    {
        if (const UConcordModelGraphComposite* CompositeNode = Cast<UConcordModelGraphComposite>(Node))
            return CompositeNode->GetComposite()->GetOutputType(GraphPinObj->GetFName());
        if (const UConcordModelGraphInstance* InstanceNode = Cast<UConcordModelGraphInstance>(Node))
            return InstanceNode->GetInstance()->GetOutputType(GraphPinObj->GetFName());
        return Vertex->GetType();
    }
}

FSlateColor SConcordModelGraphPin::GetColor() const
{
    if (TOptional<EConcordValueType> Type = GetType())
        switch (Type.GetValue())
        {
        case EConcordValueType::Int: return FColor::White;
        case EConcordValueType::Float: return FColor(50, 255, 50);
        case EConcordValueType::Error: return FColor(50, 50, 50);
        default: checkNoEntry(); return FColor::White;
        }
    else return FColor::Cyan;
}
