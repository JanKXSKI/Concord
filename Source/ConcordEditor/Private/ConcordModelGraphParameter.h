// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordModelGraphConsumer.h"
#include "ConcordParameter.h"
#include "ConcordModelGraphParameter.generated.h"

UCLASS()
class UConcordModelGraphParameter : public UConcordModelGraphConsumer
{
    GENERATED_BODY()
public:
    UConcordModelGraphParameter() { bCanRenameNode = true; }
    void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
    void OnRenameNode(const FString& NewNameString) override;
    void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    void AllocateDefaultPins() override;
    TSharedPtr<SGraphNode> CreateVisualWidget() override;
    void PostEditImport() override;
    void DestroyNode() override;

    UPROPERTY(EditAnywhere, Category = "Interface")
    bool bLocal;

    UConcordParameter* GetParameter() { return Cast<UConcordParameter>(Vertex); }
};

USTRUCT()
struct FConcordModelGraphAddNodeAction_NewParameter : public FConcordModelGraphAddNodeAction
{
    GENERATED_BODY()

    FConcordModelGraphAddNodeAction_NewParameter();
    FConcordModelGraphAddNodeAction_NewParameter(EConcordValueType InType);

    UEdGraphNode* MakeNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin) override;
private:
    EConcordValueType Type;
};
