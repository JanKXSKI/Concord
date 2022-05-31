// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphSchema.h"
#include "ConcordModel.h"
#include "ConcordCompiler.h"
#include "ConcordError.h"
#include "ConcordNativeModel.h"
#include "ConcordModelGraph.generated.h"

class UConcordVertex;

UENUM()
enum class EConcordModelGraphDistributionMode : uint8
{
    Conditional,
    Marginal,
    None
};

UCLASS()
class UConcordModelGraph : public UConcordModelGraphBase
{
    GENERATED_BODY()
public:
    UConcordModelGraph() : bLiveCoding(true) {}

    UPROPERTY(EditAnywhere, Category = "Feedback")
    EConcordModelGraphDistributionMode DistributionMode;

    UPROPERTY(EditAnywhere, Category = "Nativization")
    EConcordCycleMode NativizationCycleMode;

    UPROPERTY(EditAnywhere, Category = "Nativization")
    bool bLiveCoding;

    UFUNCTION(CallInEditor, Category = "Nativization")
    void Nativize();

    UPROPERTY(VisibleAnywhere, Category = "Nativization")
    UConcordNativeModel* NativeModel;

    UConcordModel* GetModel() const;
    void SetupShapesAndTypes();
    void SetupShapesAndTypes(TSet<UConcordVertex*>& VisitedVertices);

    static FLinearColor GetEntropyColor(const FConcordDistribution& Distribution);

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnNativizeOptionalError, const TOptional<FConcordError>&);
    FOnNativizeOptionalError OnNativizeOptionalError;

    UPROPERTY(Transient)
    TArray<class UConcordModelGraphNode*> SelectedNodes;

    TArray<UEdGraphPin*> GetSelectedPins(EEdGraphPinDirection Direction) const;
};

UCLASS(MinimalAPI)
class UConcordModelGraphSchema : public UEdGraphSchema
{
    GENERATED_BODY()
public:
    const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const override;
    void BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotifcation) const override;
    void BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const override;
    void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
private:
    bool ConnectionWouldCauseCycle(const UEdGraphPin* OutputPin, const UEdGraphPin* CycleStartInputPin) const;
};

UCLASS(CollapseCategories)
class UConcordModelGraphNode : public UEdGraphNode
{
    GENERATED_BODY()
public:
    bool CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const override;
    bool IncludeParentNodeContextMenu() const override { return true; }
    FLinearColor GetNodeTitleColor() const override { return FColor(23, 23, 23, 23); }
    FLinearColor GetNodeBodyTintColor() const override { return FColor(220, 220, 220, 220); }
    UConcordModel* GetModel() const;
    UConcordModelGraph* GetGraph() const;

    FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    void OnRenameNode(const FString& NewNameString) override;
    void OnNameChanged();
    void SyncModelName(bool bRemoveOldName);
    void PostEditImport() override;
    void PreEditChange(FProperty* PropertyAboutToChange) override;
    void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    FText GetTooltipText() const override;
    TOptional<FConcordError> SetupShapeAndType();
    TOptional<FConcordError> SetupShapeAndType(TSet<UConcordVertex*>& VisitedVertices);
    void OnInputOutputInterfaceChanged();
    static void SavePackage(UPackage* Package);

    UPROPERTY(EditAnywhere, Category = "Node")
    FName Name;

    UPROPERTY()
    UConcordVertex* Vertex;

    UPROPERTY()
    FString SerializedVertex;

    DECLARE_MULTICAST_DELEGATE(FOnNodeChanged);
    FOnNodeChanged OnNodeChanged;
private:
    FName NameToRemove;
};

USTRUCT()
struct FConcordModelGraphAddNodeAction : public FEdGraphSchemaAction
{
    GENERATED_BODY()

    FConcordModelGraphAddNodeAction() : FEdGraphSchemaAction(), LocationOffset(FVector2D::ZeroVector) {}
    FConcordModelGraphAddNodeAction(FText InNodeCategory, FText InMenuDesc, FText InToolTip, FText InTransactionText, FVector2D InLocationOffset = FVector2D::ZeroVector);
    UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
    virtual UEdGraphNode* MakeNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin) PURE_VIRTUAL(FConcordModelGraphAddNodeAction::MakeNode, return nullptr;)
private:
    UPROPERTY()
    FText TransactionText;

    UPROPERTY()
    FVector2D LocationOffset;
};

USTRUCT()
struct FConcordModelGraphFromParameterAction : public FEdGraphSchemaAction
{
    GENERATED_BODY()

    FConcordModelGraphFromParameterAction(const TArray<UEdGraphPin*>& InTargetPins);
    FConcordModelGraphFromParameterAction() : FConcordModelGraphFromParameterAction(TArray<UEdGraphPin*>()) {}
    UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
private:
    TArray<UEdGraphPin*> TargetPins;
};

USTRUCT()
struct FConcordModelGraphToOutputAction : public FEdGraphSchemaAction
{
    GENERATED_BODY()

    FConcordModelGraphToOutputAction(const TArray<UEdGraphPin*>& InSourcePins);
    FConcordModelGraphToOutputAction() : FConcordModelGraphToOutputAction(TArray<UEdGraphPin*>()) {}
    UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
private:
    TArray<UEdGraphPin*> SourcePins;
};

USTRUCT()
struct FConcordModelGraphExplodeAction : public FEdGraphSchemaAction
{
    GENERATED_BODY()

    FConcordModelGraphExplodeAction(const TArray<UEdGraphPin*>& InTargetPins);
    FConcordModelGraphExplodeAction() : FConcordModelGraphExplodeAction(TArray<UEdGraphPin*>()) {}
    UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
private:
    TArray<UEdGraphPin*> TargetPins;
};
