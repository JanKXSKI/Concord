// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordModelGraphConsumer.h"
#include "ConcordEmission.h"
#include "SConcordModelGraphSink.h"
#include "ConcordModelGraphEmission.generated.h"

UCLASS(HideCategories=("Node"))
class UConcordModelGraphEmission : public UConcordModelGraphConsumer
{
    GENERATED_BODY()
public:
    void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
    FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    TSharedPtr<SGraphNode> CreateVisualWidget() override;
    void PostEditImport() override;
    void DestroyNode() override;
    void AllocateDefaultPins() override;

    UConcordEmission* GetEmission() { return Cast<UConcordEmission>(Vertex); }
};

class SConcordModelGraphEmission : public SConcordModelGraphSink
{
private:
    void AddSinkDelegates() override;
};

USTRUCT()
struct FConcordModelGraphAddNodeAction_NewEmission : public FConcordModelGraphAddNodeAction
{
    GENERATED_BODY()

    FConcordModelGraphAddNodeAction_NewEmission();
    UEdGraphNode* MakeNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin) override;
};
