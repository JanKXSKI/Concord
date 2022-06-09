// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SConcordModelGraphNode.h"
#include "Widgets/SBoxPanel.h"

class SConcordModelGraphSink : public SConcordModelGraphNode
{
public:
    SLATE_BEGIN_ARGS(SConcordModelGraphSink)
        : _WithPinLabels(true)
        , _WithPinDefaultValues(true)
    {}
    SLATE_ARGUMENT(bool, WithPinLabels)
    SLATE_ARGUMENT(bool, WithPinDefaultValues)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, UEdGraphNode* InGraphNode);
    TSharedRef<SWidget> CreateNodeContentArea() override;
    void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;
    TSharedPtr<SGraphPin> CreatePinWidget(UEdGraphPin* Pin) const override;
    void CreateBelowPinControls(TSharedPtr<SVerticalBox> MainBox) override; // hacky way to change MainBox slot order
protected:
    virtual void AddSinkDelegates() {}
    virtual TSharedRef<SWidget> CreateAdditionalSinkContent() { return SNullWidget::NullWidget; }
private:
    TSharedPtr<SHorizontalBox> ContentBox;
    bool bWithPinLabels;
    bool bWithPinDefaultValues;
};
