// Copyright Jan Klimaschewski. All Rights Reserved.

#include "ConcordModelGraphOutput.h"
#include "ConcordModel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"

void UConcordModelGraphOutput::GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
    if (ContextMenuBuilder.FromPin &&
        ContextMenuBuilder.FromPin->Direction != EGPD_Output) return;
    ContextMenuBuilder.AddAction(MakeShared<FConcordModelGraphAddNodeAction_NewOutput>());
}

void UConcordModelGraphOutput::OnRenameNode(const FString& NewNameString)
{
    UConcordModelGraphConsumer::OnRenameNode(NewNameString);
    OnInputOutputInterfaceChanged();
}

void UConcordModelGraphOutput::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    UConcordModelGraphConsumer::PostEditChangeProperty(PropertyChangedEvent);
    if (PropertyChangedEvent.GetPropertyName() != "bDisplayOutput")
        OnInputOutputInterfaceChanged();
}

void UConcordModelGraphOutput::AllocateDefaultPins()
{
    UConcordModelGraphConsumer::AllocateDefaultPins();
    OnInputOutputInterfaceChanged();
}

TSharedPtr<SGraphNode> UConcordModelGraphOutput::CreateVisualWidget()
{
    return SNew(SConcordModelGraphOutput, this)
    .WithPinDefaultValues(true)
    .WithPinLabels(true);
}

void UConcordModelGraphOutput::PostEditImport()
{
    UConcordModelGraphConsumer::PostEditImport();
    OnInputOutputInterfaceChanged();
}

void UConcordModelGraphOutput::DestroyNode()
{
    GetModel()->Modify();
    GetModel()->Outputs.Remove(Name);
    GetModel()->OrderedOutputNames.RemoveSingle(Name);
    GetModel()->Outputs.Shrink();
    Super::DestroyNode();
    OnInputOutputInterfaceChanged();
}

void SConcordModelGraphOutput::AddSinkDelegates()
{
    UConcordModelGraphOutput* OutputNode = Cast<UConcordModelGraphOutput>(GraphNode);
    OutputNode->OnNodeChanged.AddSP(this, &SConcordModelGraphOutput::UpdateGraphNode);
    OutputNode->OnIntOutputValuesChanged.AddSP(this, &SConcordModelGraphOutput::OnIntOutputValuesChanged);
    OutputNode->OnFloatOutputValuesChanged.AddSP(this, &SConcordModelGraphOutput::OnFloatOutputValuesChanged);
}

TSharedRef<SWidget> SConcordModelGraphOutput::CreateAdditionalSinkContent()
{
    FTextBlockStyle TextBlockStyle = FTextBlockStyle::GetDefault();
    TextBlockStyle.SetFontSize(10);
    TextBlockStyle.SetTypefaceFontName("Bold");
    TextBlockStyle.SetColorAndOpacity(FLinearColor::White);
    SAssignNew(OutputValuesText, STextBlock)
    .TextStyle(&TextBlockStyle)
    .Justification(ETextJustify::Center);
    SAssignNew(ShapeLines, SConcordModelGraphOutputShapeLines, OutputValuesText);
    SAssignNew(Highlight, SConcordModelGraphOutputHighlight, OutputValuesText, Cast<UConcordModelGraphOutput>(GraphNode)->GetModel()->LatestPatternSampledFromEditor);

    return SNew(SBox)
    .Visibility(this, &SConcordModelGraphOutput::GetOutputValuesTextVisibility)
    .Padding(FMargin(0.0f, 8.0f, 8.0f, 4.0f))
    [
        SNew(SHorizontalBox)
        +SHorizontalBox::Slot()
        .AutoWidth()
        [ ShapeLines.ToSharedRef() ]
        +SHorizontalBox::Slot()
        .AutoWidth()
        [
            SNew(SOverlay)
            +SOverlay::Slot()
            [ OutputValuesText.ToSharedRef() ]
            +SOverlay::Slot()
            [ Highlight.ToSharedRef() ]
        ]
    ];
}

void SConcordModelGraphOutput::OnIntOutputValuesChanged(const TArray<int32>& Values)
{
    bool bIsNotes = CheckHighlightAndGetIsNotes(Values.Num());
    if (Values.IsEmpty()) { OutputValuesText->SetText(FText()); return; }
    FString ValuesText { "", Values.Num() * 2 };
    for (int32 ValueIndex = 0; ValueIndex < Values.Num() - 1; ++ValueIndex)
        ValuesText += LexIntValue(Values[ValueIndex], bIsNotes) + TEXT("\n");
    ValuesText += LexIntValue(Values.Last(), bIsNotes);
    OutputValuesText->SetText(FText::FromString(ValuesText));
    ShapeLines->SetShape(Cast<UConcordModelGraphOutput>(GraphNode)->GetOutput()->GetShape());
}

void SConcordModelGraphOutput::OnFloatOutputValuesChanged(const TArray<float>& Values)
{
    CheckHighlightAndGetIsNotes(0); // disables highlights
    if (Values.IsEmpty()) { OutputValuesText->SetText(FText()); return; }
    FString ValuesText { "", Values.Num() * 2 };
    for (int32 ValueIndex = 0; ValueIndex < Values.Num() - 1; ++ValueIndex)
        ValuesText += FString::Printf(TEXT("%.2f"), Values[ValueIndex]) + "\n";
    ValuesText += FString::Printf(TEXT("%.2f"), Values.Last());
    OutputValuesText->SetText(FText::FromString(ValuesText));
    ShapeLines->SetShape(Cast<UConcordModelGraphOutput>(GraphNode)->GetOutput()->GetShape());
}

bool SConcordModelGraphOutput::CheckHighlightAndGetIsNotes(int32 NumValues) const
{
    const TOptional<FConcordColumnPath> ColumnPath = FConcordColumnPath::Parse(Cast<UConcordModelGraphOutput>(GraphNode)->Name.ToString());
    Highlight->SetNeedsHighlight(ColumnPath.IsSet(), NumValues);
    if (ColumnPath) return ColumnPath.GetValue().ColumnType == FConcordColumnPath::Note;
    return false;
}

FString SConcordModelGraphOutput::LexIntValue(int32 Value, bool bIsNotes)
{
    if (Value == 0 && bIsNotes) return UTF8_TO_TCHAR("\u00B7");
    return LexToString(Value);
}

EVisibility SConcordModelGraphOutput::GetOutputValuesTextVisibility() const
{
    if (Cast<UConcordModelGraphOutput>(GraphNode)->bDisplayOutput)
        return EVisibility::Visible;
    return EVisibility::Collapsed;
}

void SConcordModelGraphOutputShapeLines::Construct(const FArguments& InArgs, TSharedPtr<STextBlock> InTextWidget)
{
    TextWidget = InTextWidget;
    Shape = { 1 };
    SetCanTick(false);
}

const float SConcordModelGraphOutputShapeLines::LineWidth = 2.0f;
const float SConcordModelGraphOutputShapeLines::LinePadding = 5.0f;
const float SConcordModelGraphOutputShapeLines::LineGap = 3.0f;

FVector2D SConcordModelGraphOutputShapeLines::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
    check(Shape.Num() > 0);
    return FVector2D((Shape.Num() - 1) * (LineWidth + LinePadding), TextWidget->GetDesiredSize().Y);
}

int32 SConcordModelGraphOutputShapeLines::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    if (Shape.Num() < 2) return LayerId;
    const int32 Num = Shape.Num() - 1;
    FSlateBrush Brush;
    PaintSegment(0, FVector2D(0.0f, 0.0f), AllottedGeometry.GetLocalSize().Y, AllottedGeometry, OutDrawElements, LayerId, Brush);
    return LayerId + 1;
}

void SConcordModelGraphOutputShapeLines::PaintSegment(int32 DimIndex, FVector2D Offset, float Height, const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FSlateBrush& Brush) const
{
    if (DimIndex >= Shape.Num() - 1) return;
    const float NextHeight = Height / Shape[DimIndex];
    const float LineLength = NextHeight - 2 * LineGap;
    const FVector2D Size = FVector2D(LineWidth, LineLength);
    for (int32 SegmentIndex = 0; SegmentIndex < Shape[DimIndex]; ++SegmentIndex)
    {
        PaintSegment(DimIndex + 1, Offset + FVector2D(LineWidth + LinePadding, 0.0f), NextHeight, AllottedGeometry, OutDrawElements, LayerId, Brush);
        const FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry(Offset + FVector2D(LinePadding * 0.5f, LineGap), Size);
        const FLinearColor Color = FLinearColor(1.0f, 1.0f, 1.0f, (SegmentIndex % 2 == 0) ? 1.0f : 0.4f);
        FSlateDrawElement::MakeBox(OutDrawElements, LayerId, PaintGeometry, &Brush, ESlateDrawEffect::None, Color);
        Offset.Y += NextHeight;
    }
}

void SConcordModelGraphOutputShapeLines::SetShape(const FConcordShape& InShape)
{
    if (InShape.Num() != Shape.Num())
        Invalidate(EInvalidateWidgetReason::Layout);
    else
        Invalidate(EInvalidateWidgetReason::Paint);
    Shape = InShape;
    if (Shape.Num() == 0) Shape = { 1 };
}

void SConcordModelGraphOutputHighlight::Construct(const FArguments& InArgs, TSharedPtr<STextBlock> InTextWidget, TWeakObjectPtr<class UConcordPattern> InPreviewPatternPtr)
{
    TextWidget = InTextWidget;
    PreviewPatternPtr = InPreviewPatternPtr;
    PreviewLine = -1;
    PreviewNumberOfLines = 1;
}

FVector2D SConcordModelGraphOutputHighlight::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
    return TextWidget->GetDesiredSize() + FVector2D(10.0f, 0.0f);
}

int32 SConcordModelGraphOutputHighlight::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    if (PreviewLine < 0 || PreviewLine >= PreviewNumberOfLines) return LayerId;
    FSlateBrush Brush;
    const FVector2D Size = FVector2D(AllottedGeometry.GetLocalSize().X, AllottedGeometry.GetLocalSize().Y / PreviewNumberOfLines);
    const FVector2D Offset = FVector2D(0.0f, PreviewLine * Size.Y);
    FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry(Offset, Size);
    FSlateDrawElement::MakeBox(OutDrawElements, LayerId, PaintGeometry, &Brush, ESlateDrawEffect::None, { 0.7f, 1.0f, 0.7f, 0.5f });
    return LayerId + 1;
}

void SConcordModelGraphOutputHighlight::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
    SLeafWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
    UConcordPattern* PreviewPattern = PreviewPatternPtr.Get();
    if (!PreviewPattern)
    {
        PreviewLine = -1;
        return;
    }
    PreviewLine = PreviewPattern->GetPreviewLine(InCurrentTime);
}

void SConcordModelGraphOutputHighlight::SetNeedsHighlight(bool bNeedsHighlight, int32 NumValues)
{
    PreviewNumberOfLines = NumValues;
    PreviewLine = -1;
    if (bNeedsHighlight && NumValues > 0) SetCanTick(true);
    else SetCanTick(false);
}

FConcordModelGraphAddNodeAction_NewOutput::FConcordModelGraphAddNodeAction_NewOutput()
    : FConcordModelGraphAddNodeAction(FText(),
                                      INVTEXT("Output"),
                                      Cast<UConcordVertex>(UConcordOutput::StaticClass()->GetDefaultObject())->GetTooltip(),
                                      INVTEXT("Add New Concord Output"))
{}

UEdGraphNode* FConcordModelGraphAddNodeAction_NewOutput::MakeNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin)
{
    FGraphNodeCreator<UConcordModelGraphOutput> NodeCreator(*ParentGraph);
    UConcordModelGraphOutput* NewOutputNode = NodeCreator.CreateUserInvokedNode();
    NewOutputNode->Name = FName("NewOutput");

    NewOutputNode->Vertex = NewObject<UConcordOutput>(NewOutputNode->GetModel(), NAME_None, RF_Transactional);
    NewOutputNode->SyncModelName(false);

    NodeCreator.Finalize();
    return NewOutputNode;
}
