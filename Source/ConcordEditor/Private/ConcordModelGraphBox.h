// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordModelGraphConsumer.h"
#include "ConcordModel.h"
#include "ConcordShape.h"
#include "ConcordBox.h"
#include "SConcordModelGraphInOutNode.h"
#include "ConcordModelGraphBox.generated.h"

class UConcordBox;

USTRUCT()
struct FConcordModelGraphBoxDimension
{
    GENERATED_BODY()

    FConcordModelGraphBoxDimension() : Size(1) {}

    UPROPERTY(EditAnywhere, meta=(ClampMin = 1), Category = "Box Shape Dimension")
    int32 Size;

    UPROPERTY(EditAnywhere, Category = "Box Shape Dimension")
    FString Name;
};

UCLASS()
class UConcordModelGraphBox : public UConcordModelGraphConsumer
{
    GENERATED_BODY()
public:
    UConcordModelGraphBox() : bEnableIndividualPins(true), bHasAllPin(false) { bCanRenameNode = true; }
    void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
    void SyncModelBox();
    void SyncPins();
    TSharedPtr<SGraphNode> CreateVisualWidget() override;
    void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    void AllocateDefaultPins() override;
    void NodeConnectionListChanged() override;
    void PostPasteNode() override;
    void DestroyNode() override;

    UPROPERTY(EditAnywhere, Category = "Box")
    TArray<FConcordModelGraphBoxDimension> DefaultShape;

    UPROPERTY(EditAnywhere, Category = "Box", meta = (ClampMin = 2))
    int32 StateCount;

    UPROPERTY(EditAnywhere, Category = "Box")
    bool bEnableIndividualPins;

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnVariationChanged, const TArrayView<const int32>&);
    FOnVariationChanged OnVariationChanged;

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnDistributionsChanged, const TArrayView<const FConcordDistribution>&);
    FOnDistributionsChanged OnDistributionsChanged;

    UConcordBox* GetBox() { return Cast<UConcordBox>(Vertex); }
    int32 GetPinIndexRV(const UEdGraphPin* Pin);
    int32 GetDefaultSize() const;
    FConcordShape GetDefaultShape() const;
private:
    int32 RVPinsNum() const;
    bool IsShapePinSet() const;

    UPROPERTY()
    bool bHasAllPin;
};

class SConcordModelGraphBox : public SConcordModelGraphInOutNode
{
public:
    SLATE_BEGIN_ARGS(SConcordModelGraphBox){}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, UEdGraphNode* InGraphNode);
    TSharedRef<SWidget> CreateNodeContentArea() override;
    void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;
    TSharedPtr<SGraphPin> CreatePinWidget(UEdGraphPin* Pin) const override;
private:
    TArray<TSharedRef<SVerticalBox>> PinBoxes;

    void AddPinBox(const FString& Name, int32 Index, int32 DimIndex);
    void OnVariationChanged(const TArrayView<const int32>& Variation);
    void OnDistributionsChanged(const TArrayView<const FConcordDistribution>& Distributions);
    bool HasAllPin() const;
};

class SConcordModelGraphBoxPin : public SGraphPin
{
public:
    SLATE_BEGIN_ARGS(SConcordModelGraphBoxPin){}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, UEdGraphPin* InPin);
    void SetValue(int32 Value);
    void SetEntropyPinColor(const FLinearColor& Color);
protected:
    TSharedPtr<STextBlock> ValueText;
    TSharedPtr<SImage> EntropyPinBack;
};

USTRUCT()
struct FConcordModelGraphAddNodeAction_NewBox : public FConcordModelGraphAddNodeAction
{
    GENERATED_BODY()

    FConcordModelGraphAddNodeAction_NewBox();
    UEdGraphNode* MakeNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin) override;
};
