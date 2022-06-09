// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordModelGraphConsumer.h"
#include "ConcordFactor.h"
#include "SConcordModelGraphSink.h"
#include "ConcordModelGraphFactor.generated.h"

UCLASS(HideCategories=("Node"))
class UConcordModelGraphFactor : public UConcordModelGraphConsumer
{
    GENERATED_BODY()
public:
    void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
    FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    TSharedPtr<SGraphNode> CreateVisualWidget() override;
    void PostEditImport() override;
    void DestroyNode() override;

    UConcordFactor* GetFactor() { return Cast<UConcordFactor>(Vertex); }
};

class SConcordModelGraphFactor : public SConcordModelGraphSink
{
private:
    void AddSinkDelegates() override;
};

USTRUCT()
struct FConcordModelGraphAddNodeAction_NewFactor : public FConcordModelGraphAddNodeAction
{
    GENERATED_BODY()

    FConcordModelGraphAddNodeAction_NewFactor();
    UEdGraphNode* MakeNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin) override;
};
