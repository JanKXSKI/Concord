// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "ConcordModelGraphDistributionViewer.h"
#include "ConcordModelGraphBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SOverlay.h"
#include "SLevelOfDetailBranchNode.h"
#include "Styling/SlateStyleRegistry.h"
#include "Algo/MaxElement.h"
#include "Widgets/Layout/SBox.h"

void UConcordModelGraphDistributionViewer::GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
    if (ContextMenuBuilder.FromPin && ContextMenuBuilder.FromPin->PinType.PinCategory != "RV") return;
    ContextMenuBuilder.AddAction(MakeShared<FConcordModelGraphAddNodeAction_NewDistributionViewer>());
}

FText UConcordModelGraphDistributionViewer::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    return INVTEXT("Distribution Viewer");
}

void UConcordModelGraphDistributionViewer::NodeConnectionListChanged()
{
    OnProbabilitiesChanged.Broadcast({}, {});
}

void UConcordModelGraphDistributionViewer::AllocateDefaultPins()
{
    CreatePin(EGPD_Input, "Viewer", "ViewerPin");
}

TSharedPtr<SGraphNode> UConcordModelGraphDistributionViewer::CreateVisualWidget()
{
    return SNew(SConcordModelGraphDistributionViewer, this)
    .WithPinLabels(false)
    .WithPinDefaultValues(false);
}

FText UConcordModelGraphDistributionViewer::GetTooltipText() const
{
    return INVTEXT("Can be connected to an individual Random Variable output on a Box to view its distribution.");
}

void SConcordModelGraphDistributionViewer::AddSinkDelegates()
{
    UConcordModelGraphDistributionViewer* ViewerNode = Cast<UConcordModelGraphDistributionViewer>(GraphNode);
    ViewerNode->OnProbabilitiesChanged.AddSP(this, &SConcordModelGraphDistributionViewer::OnProbabilitiesChanged);
}

TSharedRef<SWidget> SConcordModelGraphDistributionViewer::CreateAdditionalSinkContent()
{
    const ISlateStyle* Style = FSlateStyleRegistry::FindSlateStyle("ConcordEditorStyle");

    TSharedRef<SImage> Background = SNew(SImage)
    .Image(Style->GetBrush("DistributionViewerBackground"));

    TSharedRef<SScrollBar> BarChartScrollBar = SNew(SScrollBar)
    .Thickness(8.0f)
    .Style(&Style->GetWidgetStyle<FScrollBarStyle>("DistributionViewerScrollBar"))
    .Orientation(Orient_Horizontal);
    BarChartScrollBar->SetCursor(EMouseCursor::Default);

    TSharedRef<SScrollBox> BarChartScrollBox = SNew(SScrollBox)
    .Orientation(Orient_Horizontal)
    .ExternalScrollbar(BarChartScrollBar)
    +SScrollBox::Slot()
    [ SAssignNew(BarChart, SConcordModelGraphBarChart) ];

    TSharedRef<SVerticalBox> FullViewer = SNew(SVerticalBox)
    +SVerticalBox::Slot()
    .AutoHeight()
    [
        SNew(SOverlay)
        +SOverlay::Slot()
        .HAlign(HAlign_Left)
        .VAlign(VAlign_Top)
        [
            Background
        ]
        +SOverlay::Slot()
        .HAlign(HAlign_Left)
        .VAlign(VAlign_Top)
        .Padding(FMargin(SConcordModelGraphBarChart::Margin.X, SConcordModelGraphBarChart::Margin.Y + SConcordModelGraphBarChart::TopMargin, 0.0f, 0.0f))
        [
            SNew(SBox)
            .WidthOverride(SConcordModelGraphBarChart::Size.X - 2*SConcordModelGraphBarChart::Margin.X)
            .HAlign(HAlign_Center)
            [ BarChartScrollBox ]
        ]
        +SOverlay::Slot()
        .HAlign(HAlign_Left)
        .VAlign(VAlign_Top)
        .Padding(FMargin(12.0f, 12.0f, 0.0f, 0.0f))
        [
            SNew(STextBlock)
            .SimpleTextMode(true)
            .Text(this, &SConcordModelGraphDistributionViewer::GetMaxProbText)
        ]
        +SOverlay::Slot()
        .HAlign(HAlign_Left)
        .VAlign(VAlign_Top)
        [
            SNew(SImage)
            .Image(Style->GetBrush("DistributionViewerScreen"))
        ]
    ]
    +SVerticalBox::Slot()
    .AutoHeight()
    [ BarChartScrollBar ];

    return SNew(SBox)
    .Padding(FMargin(0.0f, 10.0f, 10.0f, 10.0f))
    [
        SNew(SLevelOfDetailBranchNode)
        .UseLowDetailSlot(this, &SConcordModelGraphDistributionViewer::UseLowDetailNodeTitles)
        .LowDetail() [ Background ]
        .HighDetail() [ FullViewer ]
    ];
}

FText SConcordModelGraphDistributionViewer::GetMaxProbText() const
{
    return FText::FromString(FString::Printf(TEXT("%.2f"), MaxProb));
}

void SConcordModelGraphDistributionViewer::OnProbabilitiesChanged(const FConcordProbabilities& Probabilities, const FConcordFactorGraphBlocks& VariationBlocks)
{
    if (!BarChart.IsValid()) return;
    if (GetNodeObj()->Pins[0]->LinkedTo.Num() == 0 || Probabilities.Num() == 0)
    {
        BarChart->SetTargetDistribution({}, 1.0f);
        return;
    }
    UEdGraphPin* LinkedPin = GetNodeObj()->Pins[0]->LinkedTo[0];
    UConcordModelGraphBox* ConnectedBoxNode = Cast<UConcordModelGraphBox>(LinkedPin->GetOwningNode());
    int32 Index = ConnectedBoxNode->GetPinIndexRV(LinkedPin);
    const FConcordDistribution& Distribution = Probabilities[VariationBlocks[ConnectedBoxNode->Name].Offset + Index];
    if (Distribution.Num() > 0) MaxProb = *Algo::MaxElement(Distribution);
    BarChart->SetTargetDistribution(Distribution, MaxProb);
}

const FVector2D SConcordModelGraphBarChart::Size = { 200.0f, 200.0f };
const FVector2D SConcordModelGraphBarChart::Margin = { 15.0f, 7.0f };
const float SConcordModelGraphBarChart::TopMargin = 23.0f;

FVector2D SConcordModelGraphBarChart::ComputeDesiredSize(float) const
{
    return FVector2D(FMath::Max(Size.X - 2*Margin.X - 8.0f, Distribution.Num() * 20.0f), Size.Y - 2*Margin.Y - TopMargin);
}

int32 SConcordModelGraphBarChart::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    if (Distribution.Num() == 0) return LayerId;

    FSlateBrush Brush;
    const float Width = AllottedGeometry.GetLocalSize().X / Distribution.Num();
    const float BarMarginX = 0.1f * Width;
    const float MaxHeight = AllottedGeometry.GetLocalSize().Y;
    for (int32 ProbIndex = 0; ProbIndex < Distribution.Num(); ++ProbIndex)
    {
        const float Prob = Distribution[ProbIndex];
        FVector2D BarSize { Width - 2 * BarMarginX, Prob * MaxHeight };
        float BarTransformX = ProbIndex * Width + BarMarginX;
        FSlateLayoutTransform BarTransform(FVector2f(BarTransformX, (1 - Prob) * MaxHeight));
        FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry(BarSize, BarTransform);
        FSlateDrawElement::MakeBox(OutDrawElements, LayerId, PaintGeometry, &Brush, ESlateDrawEffect::None, ProbIndex % 2 == 0 ? Color : 0.8f * Color);

        FPaintGeometry TextGeometry = AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(FVector2f(BarTransformX, MaxHeight - (ProbIndex < 100 ? 13.0f : 12.0f))));
        FSlateFontInfo FontInfo = { FCoreStyle::GetDefaultFont(), ProbIndex < 100 ? 8 : 7, NAME_None, FFontOutlineSettings(1) };
        FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1, TextGeometry, FString::Printf(TEXT("%d"), ProbIndex), FontInfo);
    }

    return LayerId + 2;
}

void SConcordModelGraphBarChart::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
    SLeafWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
    for (int32 ProbIndex = 0; ProbIndex < Distribution.Num(); ++ProbIndex)
        Distribution[ProbIndex] = FMath::InterpSinInOut(Distribution[ProbIndex], TargetDistribution[ProbIndex], Alpha);
    Color = FMath::Lerp(Color, TargetColor, Alpha);
    Alpha = FMath::Min(1.0f, Alpha + 0.5f * InDeltaTime);
}

void SConcordModelGraphBarChart::SetTargetDistribution(const FConcordDistribution& InDistribution, float MaxProb)
{
    TargetDistribution = InDistribution;
    TargetColor = UConcordModelGraph::GetEntropyColor(TargetDistribution);
    for (float& Prob : TargetDistribution) Prob /= MaxProb;
    Distribution.SetNumZeroed(TargetDistribution.Num());
    Alpha = 0.0f;
}

FConcordModelGraphAddNodeAction_NewDistributionViewer::FConcordModelGraphAddNodeAction_NewDistributionViewer()
    : FConcordModelGraphAddNodeAction(FText(),
                                      INVTEXT("Viewer"),
                                      INVTEXT("Connect a single random variable to this viewer to see it's inferred distribution."),
                                      INVTEXT("Add New Concord Distribution Viewer"),
                                      FVector2D(-(SConcordModelGraphBarChart::Size.X * 0.5f + 10.0f), 0.0f))
{}

UEdGraphNode* FConcordModelGraphAddNodeAction_NewDistributionViewer::MakeNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin)
{
    FGraphNodeCreator<UConcordModelGraphDistributionViewer> NodeCreator(*ParentGraph);
    UConcordModelGraphDistributionViewer* NewGraphDistributionViewer = NodeCreator.CreateUserInvokedNode();
    NodeCreator.Finalize();
    return NewGraphDistributionViewer;
}
