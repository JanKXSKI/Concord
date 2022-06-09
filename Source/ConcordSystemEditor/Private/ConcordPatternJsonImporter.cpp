// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "ConcordPatternJsonImporter.h"
#include "Misc/FileHelper.h"
#include "JsonObjectConverter.h"

void UConcordPatternJsonImporter::Import(const FString& InFilename, UConcordPattern* InOutPattern) const
{
	FString PatternString;
	if (!FFileHelper::LoadFileToString(PatternString, *InFilename)) return;
	FJsonObjectConverter::JsonObjectStringToUStruct(PatternString, &InOutPattern->PatternData);
}
