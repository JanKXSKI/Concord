// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Widgets/Layout/SScrollBox.h"
#include "EditorUndoClient.h"
#include "GraphEditor.h"
#include "ConcordModel.h"
#include "ConcordError.h"

class UConcordModel;
class UConcordModelGraph;
class UConcordModelGraphOutput;
class FConcordSampler;

class FConcordModelEditorToolkit : public FAssetEditorToolkit, public FEditorUndoClient
{
public:
    void Initialize(UConcordModel* InConcordModel);
    ~FConcordModelEditorToolkit();

    // FAssetEditorToolkit interface
    void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
    void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
    FName GetToolkitFName() const override;
    FText GetBaseToolkitName() const override;
    FString GetWorldCentricTabPrefix() const override;
    FLinearColor GetWorldCentricTabColorScale() const override;
    void OnClose() override;

    // FEditorUndoClient interface
    void PostUndo(bool bSuccess) override;
    void PostRedo(bool bSuccess) override { PostUndo(bSuccess); }
private:
    void SetupCommands();

    void CreateInternalWidgets();
    void CreateGraphEditor();
    bool ClearSavedGraphNodeErrors();
    FGraphAppearanceInfo GetGraphAppearance() const;
    void CreateDetailViews();
    void OnSelectionChanged(const TSet<UObject*>& SelectedNodes);
    void OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged);

    void ExtendToolbar();

    FString GetSelectedNodesString() const;
    void CopySelectedNodes();
    void CutSelectedNodes();
    void DeleteSelectedNodes();
    void DuplicateSelectedNodes();
    void PasteNodes();
    void PasteNodes(const FText& TransactionText);
    bool CanPasteNodes();
    void RenameSelectedNode();
    bool CanRenameSelectedNodes() const;

    void SampleVariation();
    void HandleOptionalError(const TOptional<FConcordError>& Error);
    void FocusErrorNode(const FString& ErrorMessage, const UConcordVertex* Vertex);
    void SampleVariationAndInferMarginalsPostSetup(FConcordSampler& Sampler, UConcordModelGraph* Graph);
    void SetVariationAndProbabilities(const FConcordSampler& Sampler, const FConcordProbabilities& Probabilities);
    void SetVariationOnly(const FConcordSampler& Sampler);
    void SetStatusError(const FString& Message);
    void SetStatusVariationScore(float Score);
    void SetOutputNodeValues(UConcordModelGraphOutput* OutputNode, const FConcordSampler& Sampler) const;

    void SelectedOutputsToCrate();


    enum class EDistributions
    {
        Conditional,
        Marginal,
        None
    };

    UConcordModel* ConcordModel;
    UConcordModelGraph* ConcordModelGraph;
    TSharedPtr<STextBlock> VariationScoreText;
    TSharedPtr<SGraphEditor> GraphEditor;
    TSharedPtr<SScrollBox> DetailsView;
    TSharedPtr<STextBlock> SelectionHint;
    TSharedPtr<IDetailsView> NodeDetailsView;
    TSharedPtr<IDetailsView> AdditionalDetailsView;
    TSharedPtr<FUICommandList> AdditionalGraphCommands;

    FString SerializedNodesToPaste;
};
