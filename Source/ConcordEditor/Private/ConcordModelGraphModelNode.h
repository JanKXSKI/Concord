// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordModelGraphConsumer.h"
#include "ConcordInstance.h"
#include "ConcordComposite.h"
#include "ConcordModelGraphModelNode.generated.h"

UCLASS()
class UConcordModelGraphModelNode : public UConcordModelGraphConsumer
{
    GENERATED_BODY()
public:
    UConcordModelGraphModelNode() { bCanRenameNode = true; }
    FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    TSharedPtr<SGraphNode> CreateVisualWidget() override;
    void AllocateDefaultPins() override;
    void PostEditImport() override;
    void PostDuplicate(bool bDuplicateForPie) override;
    void AddInputOutputChangedDelegate();

    UFUNCTION()
    void SyncPins();

    void DestroyNode() override;
private:
    virtual UConcordModelBase* GetReferencedModel() const PURE_VIRTUAL(UConcordModelGraphModelNode::GetReferencedModel, return nullptr;)
    virtual void PreCreateVisualWidget() {};
    virtual void PreSyncPins() {};
};

UCLASS()
class UConcordModelGraphComposite : public UConcordModelGraphModelNode
{
    GENERATED_BODY()
public:
    void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
    void DestroyNode() override;

    UPROPERTY(VisibleAnywhere, Category = "Composite")
    UConcordModel* Model;

    UConcordComposite* GetComposite() { return Cast<UConcordComposite>(Vertex); }
    const UConcordComposite* GetComposite() const { return Cast<UConcordComposite>(Vertex); }
private:
    UConcordModelBase* GetReferencedModel() const override { return Model; }
};

USTRUCT()
struct FConcordModelGraphAddNodeAction_NewComposite : public FConcordModelGraphAddNodeAction
{
    GENERATED_BODY()

    FConcordModelGraphAddNodeAction_NewComposite();
    FConcordModelGraphAddNodeAction_NewComposite(FAssetData InModelAssetData);
    UEdGraphNode* MakeNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin) override;

    FAssetData ModelAssetData;
};

UCLASS()
class UConcordModelGraphInstance : public UConcordModelGraphModelNode
{
    GENERATED_BODY()
public:
    void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
    void DestroyNode() override;

    UPROPERTY(VisibleAnywhere, Category = "Instance")
    UConcordModelBase* Model;

    UConcordInstance* GetInstance() { return Cast<UConcordInstance>(Vertex); }
    const UConcordInstance* GetInstance() const { return Cast<UConcordInstance>(Vertex); }
private:
    UConcordModelBase* GetReferencedModel() const override { return Model; }
    void PreCreateVisualWidget() override;
    void PreSyncPins() override;
};

USTRUCT()
struct FConcordModelGraphAddNodeAction_NewInstance : public FConcordModelGraphAddNodeAction
{
    GENERATED_BODY()

    FConcordModelGraphAddNodeAction_NewInstance();
    FConcordModelGraphAddNodeAction_NewInstance(FAssetData InModelAssetData);
    UEdGraphNode* MakeNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin) override;

    FAssetData ModelAssetData;
};
