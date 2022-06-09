// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordValue.h"
#include "KismetPins/SGraphPinNum.h"

class SConcordModelGraphPin : public SGraphPin
{
public:
    SLATE_BEGIN_ARGS(SConcordModelGraphPin)
        : _WithLabel(true)
        , _WithDefaultValue(true)
    {}
    SLATE_ARGUMENT(bool, WithLabel)
    SLATE_ARGUMENT(bool, WithDefaultValue)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, UEdGraphPin* InPin);
	TSharedRef<SWidget>	GetDefaultValueWidget() override;
    TSharedRef<SWidget> GetLabelWidget(const FName& InPinLabelStyle) override;
private:
    TOptional<double> GetNumericValue() const;
    void SetNumericValue(double InValue, ETextCommit::Type CommitType);
    bool ShouldLexInt(double InValue) const;
    TOptional<EConcordValueType> GetType() const;
    FSlateColor GetColor() const;
};
