// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordModelGraphTikzExporter.generated.h"

class UConcordModelGraph;

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

    UPROPERTY(EditAnywhere, meta = (ClampMin = 0, ClampMax = 1))
    float DistanceScale;
private:
    void ExportInteral(const UConcordModelGraph* Graph);
};
