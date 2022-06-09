// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordPattern.h"
#include "ConcordPatternJsonImporter.generated.h"

UCLASS()
class CONCORDSYSTEMEDITOR_API UConcordPatternJsonImporter : public UConcordPatternImporter
{
    GENERATED_BODY()
public:
    void Import(const FString& InFilename, UConcordPattern* InOutPattern) const override;
};
