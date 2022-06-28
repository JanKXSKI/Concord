// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordPattern.h"
#include "IDetailCustomization.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ConcordPatternCustomization.generated.h"

class FConcordPatternCustomization : public IDetailCustomization
{
public:
	void CustomizeDetails(IDetailLayoutBuilder& LayoutBuilder) override;
	static TSharedRef<IDetailCustomization> MakeInstance() { return MakeShared<FConcordPatternCustomization>(); }
private:
	void Play() const;
	void Stop() const;
	void ExportMidi() const;

	TWeakObjectPtr<UConcordPattern> CustomizedPattern;
};

UCLASS()
class UConcordPatternUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Concord Pattern Utilities")
	static void ExportMidi(UConcordPattern* Pattern, const FString& Filename);
};
