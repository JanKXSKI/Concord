// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordPattern.h"
#include "IDetailCustomization.h"

class FConcordPatternCustomization : public IDetailCustomization
{
public:
	void CustomizeDetails(IDetailLayoutBuilder& LayoutBuilder) override;
	static TSharedRef<IDetailCustomization> MakeInstance() { return MakeShared<FConcordPatternCustomization>(); }
private:
	void Play() const;
	void Stop() const;

	TWeakObjectPtr<UConcordPattern> CustomizedPattern;
};
