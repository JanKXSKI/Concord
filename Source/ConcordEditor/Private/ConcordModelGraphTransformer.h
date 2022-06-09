// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordModelGraphConsumer.h"
#include "ConcordTransformer.h"
#include "SConcordModelGraphNode.h"
#include "SConcordModelGraphPin.h"
#include "ConcordModelGraphTransformer.generated.h"

UCLASS(HideCategories=("Node"))
class UConcordModelGraphTransformer : public UConcordModelGraphConsumer
{
    GENERATED_BODY()
public:
    void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
    FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    TSharedPtr<SGraphNode> CreateVisualWidget() override;
    void AllocateDefaultPins() override;

    UConcordTransformer* GetTransformer() { return Cast<UConcordTransformer>(Vertex); }
};

USTRUCT()
struct FConcordModelGraphAddNodeAction_NewTransformer : public FConcordModelGraphAddNodeAction
{
    GENERATED_BODY()

    FConcordModelGraphAddNodeAction_NewTransformer();
    FConcordModelGraphAddNodeAction_NewTransformer(TSubclassOf<UConcordTransformer> InClass, const TArray<UEdGraphPin*>& InSourcePins);
    UEdGraphNode* MakeNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin) override;

    TSubclassOf<UConcordTransformer> Class;
    TArray<UEdGraphPin*> SourcePins;
};
