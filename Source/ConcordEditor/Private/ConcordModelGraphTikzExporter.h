// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordModelGraphTikzExporter.generated.h"

class UConcordModelGraph;
class UConcordModelGraphNode;
class UEdGraph;

UCLASS()
class UConcordModelGraphTikzExporter : public UObject
{
    GENERATED_BODY()
public:
    static void Export(const UConcordModelGraph* Graph);

    UConcordModelGraphTikzExporter();

    UPROPERTY(EditAnywhere)
    FDirectoryPath Directory;

    UPROPERTY(EditAnywhere)
    FString FileName;

    UPROPERTY(EditAnywhere, meta = (ClampMin = -5, ClampMax = 5))
    float DistanceScale;

    static FVector2D GetCenter(const UEdGraph* Graph);
private:
    void ExportInteral(const UConcordModelGraph* Graph);

    void WriteHeader();
    void WriteNode(const UEdGraphNode* Node);
    void WriteOutgoingEdges(const UEdGraphNode* Node);
    void WriteDefaultValues(const UEdGraphNode* Node);
    void WriteFooter();

    void WriteID(const UEdGraphNode* Node, const TCHAR* Suffix, TOptional<EEdGraphPinDirection> PinDirection = {});
    void WritePinID(const UEdGraphPin* Pin, const TCHAR* Suffix = TEXT(""));
    void WriteTitle(const UEdGraphNode* Node);
    void WriteInputOutputPinPairs(const UEdGraphNode* Node, int32 InputPinIndex = 0, int32 OutputPinIndex = 0);
    void WritePin(const UEdGraphPin* Pin);
    void WritePinPlaceholder(EEdGraphPinDirection Direction);
    void WriteDetails(const UEdGraphNode* Node);

    FString Buffer;
    FVector2D Center;
};
