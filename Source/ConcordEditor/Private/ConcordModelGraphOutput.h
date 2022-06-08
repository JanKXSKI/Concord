// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordModelGraphConsumer.h"
#include "ConcordOutput.h"
#include "SConcordModelGraphSink.h"
#include "ConcordModelGraphOutput.generated.h"

UCLASS()
class UConcordModelGraphOutput : public UConcordModelGraphConsumer
{
    GENERATED_BODY()
public:
    UConcordModelGraphOutput() : bDisplayOutput(false) { bCanRenameNode = true; bDisplayOutput = true; }
    void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
    void OnRenameNode(const FString& NewNameString) override;
    void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    void AllocateDefaultPins() override;
    TSharedPtr<SGraphNode> CreateVisualWidget() override;
    void PostEditImport() override;
    void DestroyNode() override;

    UPROPERTY(EditAnywhere, Category = "Output")
    bool bDisplayOutput;

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnIntOutputValuesChanged, const TArray<int32>&);
    FOnIntOutputValuesChanged OnIntOutputValuesChanged;
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnFloatOutputValuesChanged, const TArray<float>&);
    FOnFloatOutputValuesChanged OnFloatOutputValuesChanged;
    TArray<FConcordValue> CurrentOutputValues;

    UConcordOutput* GetOutput() { return Cast<UConcordOutput>(Vertex); }
};

class SConcordModelGraphOutputShapeLines : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SConcordModelGraphOutputShapeLines){}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, TSharedPtr<STextBlock> InTextWidget);
    FVector2D ComputeDesiredSize(float) const override;
    int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
    void SetShape(const FConcordShape& InShape);
private:
    void PaintSegment(int32 DimIndex, FVector2D Offset, float Height, const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FSlateBrush& Brush) const;

    TSharedPtr<STextBlock> TextWidget;
    TArray<int32> Shape;

    static const float LineWidth;
    static const float LinePadding;
    static const float LineGap;
};

class SConcordModelGraphOutputHighlight : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SConcordModelGraphOutputHighlight){}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, TSharedPtr<STextBlock> InTextWidget, TWeakObjectPtr<class UConcordPattern> InPreviewPatternPtr);
    FVector2D ComputeDesiredSize(float) const override;
    int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
    void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

    void SetNeedsHighlight(bool bNeedsHighlight, int32 NumValues);
private:
    TSharedPtr<STextBlock> TextWidget;
    TWeakObjectPtr<class UConcordPattern> PreviewPatternPtr;
    int32 PreviewLine, PreviewNumberOfLines;
};

class SConcordModelGraphOutput : public SConcordModelGraphSink
{
private:
    void AddSinkDelegates() override;
    TSharedRef<SWidget> CreateAdditionalSinkContent() override;
    void OnIntOutputValuesChanged(const TArray<int32>& Values);
    void OnFloatOutputValuesChanged(const TArray<float>& Values);
    bool CheckHighlightAndGetIsNotes(int32 NumValues) const;
    static FString LexIntValue(int32 Value, bool bIsNotes);
    EVisibility GetOutputValuesTextVisibility() const;

    TSharedPtr<STextBlock> OutputValuesText;
    TSharedPtr<SConcordModelGraphOutputShapeLines> ShapeLines;
    TSharedPtr<SConcordModelGraphOutputHighlight> Highlight;
};

USTRUCT()
struct FConcordModelGraphAddNodeAction_NewOutput : public FConcordModelGraphAddNodeAction
{
    GENERATED_BODY()

    FConcordModelGraphAddNodeAction_NewOutput();
    UEdGraphNode* MakeNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin) override;
};
