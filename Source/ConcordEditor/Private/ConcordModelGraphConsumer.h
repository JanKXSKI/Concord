// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordModelGraph.h"
#include "ConcordModelGraphConsumer.generated.h"

UCLASS()
class UConcordModelGraphConsumer : public UConcordModelGraphNode
{
    GENERATED_BODY()
public:
    FText GetPinDisplayName(const UEdGraphPin* Pin) const override;
    void AllocateDefaultPins() override;
    void PinDefaultValueChanged(UEdGraphPin* Pin) override;
    void NodeConnectionListChanged() override;
    void PostPasteNode() override;
    void SyncModelConnections();
};
