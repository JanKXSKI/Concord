// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordModelGraph.h"
#include "FactorGraph/ConcordFactorGraph.h"
#include "SConcordModelGraphSink.h"
#include "ConcordModelGraphDistributionViewer.generated.h"

UCLASS(HideCategories=("Node"))
class UConcordModelGraphDistributionViewer : public UConcordModelGraphNode
{
    GENERATED_BODY()
public:
    void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
    FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    void NodeConnectionListChanged() override;
    void AllocateDefaultPins() override;
    TSharedPtr<SGraphNode> CreateVisualWidget() override;
    FText GetTooltipText() const override;

    DECLARE_MULTICAST_DELEGATE_TwoParams(FOnProbabilitiesChanged, const FConcordProbabilities&, const FConcordFactorGraphBlocks&);
    FOnProbabilitiesChanged OnProbabilitiesChanged;
};

class SConcordModelGraphDistributionViewer : public SConcordModelGraphSink
{
private:
    void AddSinkDelegates() override;
    TSharedRef<SWidget> CreateAdditionalSinkContent() override;

    FText GetMaxProbText() const;
    void OnProbabilitiesChanged(const FConcordProbabilities& Probabilities, const FConcordFactorGraphBlocks& VariationBlocks);

    TSharedPtr<class SConcordModelGraphBarChart> BarChart;
    float MaxProb;
};

class SConcordModelGraphBarChart : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SConcordModelGraphBarChart){}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs) {}
    FVector2D ComputeDesiredSize(float) const override;
    int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
    void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

    void SetTargetDistribution(const FConcordDistribution& InDistribution, float MaxProb);

    static const FVector2D Size;
    static const FVector2D Margin;
    static const float TopMargin;
private:
    FConcordDistribution Distribution;
    FConcordDistribution TargetDistribution;
    FLinearColor Color;
    FLinearColor TargetColor;
    float Alpha;
};

USTRUCT()
struct FConcordModelGraphAddNodeAction_NewDistributionViewer : public FConcordModelGraphAddNodeAction
{
    GENERATED_BODY()

    FConcordModelGraphAddNodeAction_NewDistributionViewer();
    UEdGraphNode* MakeNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin) override;
};
