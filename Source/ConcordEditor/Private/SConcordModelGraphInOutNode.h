// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SConcordModelGraphPin.h"
#include "SConcordModelGraphNode.h"

class SConcordModelGraphInOutNode : public SConcordModelGraphNode
{
public:
    SLATE_BEGIN_ARGS(SConcordModelGraphInOutNode)
        : _bWithDefaultValues(true)
        , _bWithOutputLabels(false)
        , _bTitleOnTop(false)
    {}
    SLATE_ARGUMENT(bool, bWithDefaultValues)
    SLATE_ARGUMENT(bool, bWithOutputLabels)
    SLATE_ARGUMENT(bool, bTitleOnTop)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, UEdGraphNode* InGraphNode);
    TSharedRef<SWidget> CreateNodeContentArea() override;
    TSharedRef<SWidget> CreateTitleWidget(TSharedPtr<SNodeTitle> NodeTitle) override;
    void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;
    TSharedPtr<SGraphPin> CreatePinWidget(UEdGraphPin* Pin) const override;
    void CreateBelowPinControls(TSharedPtr<SVerticalBox> MainBox) override; // hacky way to remove title
    const FSlateBrush* GetNodeBodyBrush() const override;
protected:
    bool bWithDefaultValues;
    bool bWithOutputLabels;
    bool bTitleOnTop;
};
