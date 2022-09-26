// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "ConcordModelEditorToolkit.h"
#include "ConcordModelEditorCommands.h"
#include "ConcordModelGraphBox.h"
#include "ConcordModelGraphDistributionViewer.h"
#include "ConcordModelGraphConsumer.h"
#include "ConcordModelGraphOutput.h"
#include "ConcordModelGraphParameter.h"
#include "ConcordFactor.h"
#include "Sampler/ConcordSampler.h"
#include "Sampler/ConcordExactSampler.h"
#include "ScopedTransaction.h"
#include "EdGraph/EdGraph.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/STextComboBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Logging/TokenizedMessage.h"
#include "ISinglePropertyView.h"
#include "EdGraphUtilities.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Exporters/Exporter.h"
#include "UObject/PropertyPortFlags.h"
#include "UnrealExporter.h"
#include "GraphEditorActions.h"
#include "ConcordCrate.h"
#include "ConcordCrateFactory.h"
#include "AssetToolsModule.h"
#include "Components/AudioComponent.h"
#include "ConcordModelGraphTikzExporter.h"

void FConcordModelEditorToolkit::Initialize(UConcordModel* InConcordModel)
{
    ConcordModel = InConcordModel;
    ConcordModelGraph = Cast<UConcordModelGraph>(ConcordModel->Graph);
    ConcordModelGraph->OnNativizeOptionalError.AddSP(this, &FConcordModelEditorToolkit::HandleOptionalError);
    GEditor->RegisterForUndo(this);
    SetupCommands();
    CreateInternalWidgets();
    ExtendToolbar();

    const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("ConcordModelEditorLayout")
        ->AddArea
        (
            FTabManager::NewPrimaryArea()
            ->SetOrientation(Orient_Horizontal)
            ->Split
            (
                FTabManager::NewStack()
                ->SetSizeCoefficient(0.2f)
                ->AddTab("ConcordModelEditor_Details", ETabState::OpenedTab)
            )
            ->Split
            (
                FTabManager::NewStack()
                ->SetHideTabWell(true)
                ->SetSizeCoefficient(0.8f)
                ->AddTab("ConcordModelEditor_Main", ETabState::OpenedTab)
            )
        );

    InitAssetEditor(EToolkitMode::Standalone,
                    TSharedPtr<IToolkitHost>(),
                    "ConcordModelEditor",
                    Layout,
                    true, // bCreateDefaultStandaloneMenu
                    true, // bCreateDefaultToolbar
                    ConcordModel,
                    false, // bInIsToolbarFocusable
                    false); // bInUseSmallToolbarIcons
}

FConcordModelEditorToolkit::~FConcordModelEditorToolkit()
{
    GEditor->UnregisterForUndo(this);
}

void FConcordModelEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(INVTEXT("Concord Model Editor"));
    auto WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

    FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

    InTabManager->RegisterTabSpawner("ConcordModelEditor_Main", FOnSpawnTab::CreateLambda([&](const FSpawnTabArgs& Args)
    {
        return SNew(SDockTab)
        .Label(INVTEXT("Model"))
        [ GraphEditor.ToSharedRef() ];
    }))
    .SetDisplayName(INVTEXT("Model Editor"))
    .SetGroup(WorkspaceMenuCategoryRef)
    .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.EventGraph_24x"));

    InTabManager->RegisterTabSpawner("ConcordModelEditor_Details", FOnSpawnTab::CreateLambda([&](const FSpawnTabArgs& Args)
    {
        return SNew(SDockTab)
        .Label(INVTEXT("Details"))
        [ DetailsView.ToSharedRef() ];
    }))
    .SetDisplayName(INVTEXT("Details"))
    .SetGroup(WorkspaceMenuCategoryRef)
    .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FConcordModelEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    InTabManager->UnregisterTabSpawner("ConcordModelEditor_Main");
    InTabManager->UnregisterTabSpawner("ConcordModelEditor_Details");
}

FName FConcordModelEditorToolkit::GetToolkitFName() const
{
    return FName("ConcordModelEditor");
}

FText FConcordModelEditorToolkit::GetBaseToolkitName() const
{
    return INVTEXT("Concord Model Editor");
}

FString FConcordModelEditorToolkit::GetWorldCentricTabPrefix() const
{
    return FString("Concord Model ");
}

FLinearColor FConcordModelEditorToolkit::GetWorldCentricTabColorScale() const
{
    return FLinearColor();
}

void FConcordModelEditorToolkit::OnClose()
{
    if (!ConcordModel) return;
    if (!ConcordModel->LatestPatternSampledFromEditor) return;
    ConcordModel->LatestPatternSampledFromEditor->StopPreview();
    if (!GEditor->GetPreviewAudioComponent()) return;
    if (GEditor->GetPreviewAudioComponent()->Sound == ConcordModel->LatestPatternSampledFromEditor->PreviewSound)
        GEditor->ResetPreviewAudioComponent();
}

void FConcordModelEditorToolkit::PostUndo(bool bSuccess)
{
    GraphEditor->NotifyGraphChanged();
    NodeDetailsView->ForceRefresh();
    AdditionalDetailsView->ForceRefresh();
}

void FConcordModelEditorToolkit::SetupCommands()
{
    FConcordModelEditorCommands::Register();

    AdditionalGraphCommands = MakeShared<FUICommandList>();

    AdditionalGraphCommands->MapAction(FGenericCommands::Get().SelectAll,
                                       FExecuteAction::CreateLambda([this](){ GraphEditor->SelectAllNodes(); }));
    AdditionalGraphCommands->MapAction(FGenericCommands::Get().Copy,
                                       FExecuteAction::CreateSP(this, &FConcordModelEditorToolkit::CopySelectedNodes));
    AdditionalGraphCommands->MapAction(FGenericCommands::Get().Cut,
                                       FExecuteAction::CreateSP(this, &FConcordModelEditorToolkit::CutSelectedNodes));
    AdditionalGraphCommands->MapAction(FGenericCommands::Get().Paste,
                                       FExecuteAction::CreateSP(this, &FConcordModelEditorToolkit::PasteNodes),
                                       FCanExecuteAction::CreateSP(this, &FConcordModelEditorToolkit::CanPasteNodes));
    AdditionalGraphCommands->MapAction(FGenericCommands::Get().Delete,
                                       FExecuteAction::CreateSP(this, &FConcordModelEditorToolkit::DeleteSelectedNodes));
    AdditionalGraphCommands->MapAction(FGenericCommands::Get().Duplicate,
                                       FExecuteAction::CreateSP(this, &FConcordModelEditorToolkit::DuplicateSelectedNodes));
    AdditionalGraphCommands->MapAction(FGenericCommands::Get().Rename,
                                       FExecuteAction::CreateLambda([this] { RenameSelectedNode(); }),
                                       FCanExecuteAction::CreateLambda([this]() { return CanRenameSelectedNodes(); }));
    // Alignment Commands
    AdditionalGraphCommands->MapAction(FGraphEditorCommands::Get().AlignNodesTop,
                                       FExecuteAction::CreateLambda([this]() { GraphEditor->OnAlignTop(); }));

    AdditionalGraphCommands->MapAction(FGraphEditorCommands::Get().AlignNodesMiddle,
                                       FExecuteAction::CreateLambda([this]() { GraphEditor->OnAlignMiddle(); }));

    AdditionalGraphCommands->MapAction(FGraphEditorCommands::Get().AlignNodesBottom,
                                       FExecuteAction::CreateLambda([this]() { GraphEditor->OnAlignBottom(); }));

    AdditionalGraphCommands->MapAction(FGraphEditorCommands::Get().AlignNodesLeft,
                                       FExecuteAction::CreateLambda([this]() { GraphEditor->OnAlignLeft(); }));

    AdditionalGraphCommands->MapAction(FGraphEditorCommands::Get().AlignNodesCenter,
                                       FExecuteAction::CreateLambda([this]() { GraphEditor->OnAlignCenter(); }));

    AdditionalGraphCommands->MapAction(FGraphEditorCommands::Get().AlignNodesRight,
                                       FExecuteAction::CreateLambda([this]() { GraphEditor->OnAlignRight(); }));

    AdditionalGraphCommands->MapAction(FGraphEditorCommands::Get().StraightenConnections,
                                       FExecuteAction::CreateLambda([this]() { GraphEditor->OnStraightenConnections(); }));

    // Distribution Commands
    AdditionalGraphCommands->MapAction(FGraphEditorCommands::Get().DistributeNodesHorizontally,
                                       FExecuteAction::CreateLambda([this]() { GraphEditor->OnDistributeNodesH(); }));

    AdditionalGraphCommands->MapAction(FGraphEditorCommands::Get().DistributeNodesVertically,
                                       FExecuteAction::CreateLambda([this]() { GraphEditor->OnDistributeNodesV(); }));


    ToolkitCommands->MapAction(FConcordModelEditorCommands::Get().SampleVariation,
                               FExecuteAction::CreateSP(this, &FConcordModelEditorToolkit::SampleVariation));
    ToolkitCommands->MapAction(FConcordModelEditorCommands::Get().SelectedOutputsToCrate,
                               FExecuteAction::CreateSP(this, &FConcordModelEditorToolkit::SelectedOutputsToCrate));
    ToolkitCommands->MapAction(FConcordModelEditorCommands::Get().ExportTikz,
                               FExecuteAction::CreateSP(this, &FConcordModelEditorToolkit::ExportTikz));
}

void FConcordModelEditorToolkit::CreateInternalWidgets()
{
    CreateGraphEditor();
    CreateDetailViews();
}

void FConcordModelEditorToolkit::CreateGraphEditor()
{
    SGraphEditor::FGraphEditorEvents GraphEvents;
    GraphEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FConcordModelEditorToolkit::OnSelectionChanged);
    GraphEvents.OnTextCommitted = FOnNodeTextCommitted::CreateSP(this, &FConcordModelEditorToolkit::OnNodeTitleCommitted);

    SAssignNew(GraphEditor, SGraphEditor)
    .AdditionalCommands(AdditionalGraphCommands)
    .Appearance(this, &FConcordModelEditorToolkit::GetGraphAppearance)
    .GraphEvents(GraphEvents)
    .GraphToEdit(ConcordModel->Graph);

    ClearSavedGraphNodeErrors();
}

bool FConcordModelEditorToolkit::ClearSavedGraphNodeErrors()
{
    bool bHadErrors = false;
    for (UEdGraphNode* Node : GraphEditor->GetCurrentGraph()->Nodes)
        if (UConcordModelGraphConsumer* ConsumerNode = Cast<UConcordModelGraphConsumer>(Node))
        {
            bHadErrors |= ConsumerNode->bHasCompilerMessage;
            ConsumerNode->ClearCompilerMessage();
        }
    return bHadErrors;
}

FGraphAppearanceInfo FConcordModelEditorToolkit::GetGraphAppearance() const
{
    FGraphAppearanceInfo AppearanceInfo;
    if (!ConcordModel) return AppearanceInfo;
    AppearanceInfo.CornerText = FText::FromString(ConcordModel->GetName());
    return AppearanceInfo;
}

void FConcordModelEditorToolkit::CreateDetailViews()
{
    FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

    FDetailsViewArgs Args;
    Args.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Hide;
    Args.NameAreaSettings = FDetailsViewArgs::HideNameArea;

    TSharedRef<IDetailsView> ModelDetailsView = PropertyModule.CreateDetailView(Args);
    ModelDetailsView->HideFilterArea(true);
    ModelDetailsView->SetObject(ConcordModel);

    TSharedRef<IDetailsView> GraphDetailsView = PropertyModule.CreateDetailView(Args);
    GraphDetailsView->HideFilterArea(true);
    GraphDetailsView->SetObject(ConcordModel->Graph);

    SAssignNew(SelectionHint, STextBlock)
    .Text(INVTEXT("Select an object to view details."))
    .AutoWrapText(true)
    .Justification(ETextJustify::Center)
    .Margin(FMargin(0.0f, 10.0f, 0.0f, 0.0f));

    NodeDetailsView = PropertyModule.CreateDetailView(Args);
    NodeDetailsView->HideFilterArea(true);
    NodeDetailsView->ForceRefresh();

    AdditionalDetailsView = PropertyModule.CreateDetailView(Args);
    AdditionalDetailsView->HideFilterArea(true);
    AdditionalDetailsView->ForceRefresh();

    SAssignNew(DetailsView, SScrollBox)
    +SScrollBox::Slot()
    [
        SNew(SVerticalBox)
        +SVerticalBox::Slot()
        .AutoHeight()
        [ ModelDetailsView ]
        +SVerticalBox::Slot()
        .AutoHeight()
        [ GraphDetailsView ]
        +SVerticalBox::Slot()
        .AutoHeight()
        [ SelectionHint.ToSharedRef() ]
        +SVerticalBox::Slot()
        .AutoHeight()
        [ NodeDetailsView.ToSharedRef() ]
        +SVerticalBox::Slot()
        .AutoHeight()
        [ AdditionalDetailsView.ToSharedRef() ]
    ];
}

void FConcordModelEditorToolkit::OnSelectionChanged(const TSet<UObject*>& SelectedNodes)
{
    AdditionalDetailsView->SetObjects(TArray<UObject*>());
    TArray<UObject*> SelectedNodesArray = SelectedNodes.Array();
    switch (SelectedNodesArray.Num())
    {
    case 0:
        SelectionHint->SetVisibility(EVisibility::Visible);
        SelectionHint->SetText(INVTEXT("Select an object to view details."));
        break;
    case 1:
        SelectionHint->SetVisibility(EVisibility::Collapsed);
        break;
    default:
        SelectionHint->SetVisibility(EVisibility::Visible);
        SelectionHint->SetText(INVTEXT("Editing multiple selected nodes is currently not supported."));
        SelectedNodesArray.Empty();
    }
    if (SelectedNodesArray.Num() == 1)
    {
        if (UConcordModelGraphNode* ConcordNode = Cast<UConcordModelGraphNode>(SelectedNodesArray[0]))
        {
            SelectedNodesArray[0] = ConcordNode->Vertex;
            AdditionalDetailsView->SetObjects(TArray<UObject*>({ ConcordNode }));
        }
    }
    NodeDetailsView->SetObjects(SelectedNodesArray);

    ConcordModelGraph->SelectedNodes.Reset();
    for (UObject* Object : SelectedNodes)
        if (UConcordModelGraphNode* Node = Cast<UConcordModelGraphNode>(Object))
            ConcordModelGraph->SelectedNodes.Add(Node);
}

void FConcordModelEditorToolkit::OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged)
{
    const FScopedTransaction Transaction(TEXT(""), INVTEXT("Rename Node"), NodeBeingChanged);
    NodeBeingChanged->Modify();
    ConcordModel->Modify();
    NodeBeingChanged->OnRenameNode(NewText.ToString());
    NodeDetailsView->ForceRefresh();
    AdditionalDetailsView->ForceRefresh();
}

void FConcordModelEditorToolkit::ExtendToolbar()
{
    FTextBlockStyle TextBlockStyle = FTextBlockStyle::GetDefault();
    TextBlockStyle.SetFontSize(11);
    TextBlockStyle.SetTypefaceFontName("Bold");
    SAssignNew(VariationScoreText, STextBlock)
    .TextStyle(&TextBlockStyle)
    .Visibility(EVisibility::Collapsed);

    TSharedPtr<FExtender> ToolbarExtender = MakeShared<FExtender>();
    ToolbarExtender->AddToolBarExtension("Asset",
                                         EExtensionHook::After,
                                         GetToolkitCommands(),
                                         FToolBarExtensionDelegate::CreateLambda([this](FToolBarBuilder& ToolbarBuilder)
    {
        ToolbarBuilder.BeginSection("Sampler");
        ToolbarBuilder.AddToolBarButton(FConcordModelEditorCommands::Get().SampleVariation);
        ToolbarBuilder.AddWidget(SNew(SBox).VAlign(VAlign_Center)[VariationScoreText.ToSharedRef()]);
        ToolbarBuilder.AddToolBarButton(FConcordModelEditorCommands::Get().SelectedOutputsToCrate);
        ToolbarBuilder.AddToolBarButton(FConcordModelEditorCommands::Get().ExportTikz);
        ToolbarBuilder.EndSection();
    }));
    AddToolbarExtender(ToolbarExtender);
}

FString FConcordModelEditorToolkit::GetSelectedNodesString() const
{
    FString NodeString;
    const FGraphPanelSelectionSet SelectedNodes = GraphEditor->GetSelectedNodes();
    const FExportObjectInnerContext Context;
    TMap<UConcordModelGraphNode*, TStrongObjectPtr<UConcordVertex>> VertexCache;
    for (UObject* NodeObj : SelectedNodes)
    {
        FStringOutputDevice Archive;
        UConcordModelGraphNode* Node = Cast<UConcordModelGraphNode>(NodeObj);

        TArray<TStrongObjectPtr<UConcordTransformer>> ConnectedTransformerCache;
        for (int32 ConnectedTransformerIndex = 0; ConnectedTransformerIndex < Node->Vertex->ConnectedTransformers.Num(); ++ConnectedTransformerIndex)
        {
            ConnectedTransformerCache.Add(TStrongObjectPtr(Node->Vertex->ConnectedTransformers[ConnectedTransformerIndex]));
            Node->Vertex->ConnectedTransformers[ConnectedTransformerIndex] = nullptr; // stops warnings on pasting
        }

        UExporter::ExportToOutputDevice(&Context, Node->Vertex, NULL, Archive, TEXT("copy"), 0, PPF_Copy|PPF_Delimited);
        Node->SerializedVertex = Archive;

        for (int32 ConnectedTransformerIndex = 0; ConnectedTransformerIndex < Node->Vertex->ConnectedTransformers.Num(); ++ConnectedTransformerIndex)
            Node->Vertex->ConnectedTransformers[ConnectedTransformerIndex] = ConnectedTransformerCache[ConnectedTransformerIndex].Get();

        VertexCache.Add(Node, TStrongObjectPtr(Node->Vertex));
        Node->Vertex = nullptr; // stops warnings on pasting
    }
    FEdGraphUtilities::ExportNodesToText(SelectedNodes, NodeString);
    for (const auto& NodeVertexPair : VertexCache)
    {
        NodeVertexPair.Key->SerializedVertex.Empty();
        NodeVertexPair.Key->Vertex = NodeVertexPair.Value.Get();
    }
    return MoveTemp(NodeString);
}

void FConcordModelEditorToolkit::CopySelectedNodes()
{
    FPlatformApplicationMisc::ClipboardCopy(*GetSelectedNodesString());
}

void FConcordModelEditorToolkit::CutSelectedNodes()
{
    CopySelectedNodes();
    DeleteSelectedNodes();
}

void FConcordModelEditorToolkit::DeleteSelectedNodes()
{
    const FScopedTransaction Transaction(INVTEXT("Delete Selected Concord Node(s)"));
    ConcordModel->Modify();
    ConcordModel->Graph->Modify();
    const FGraphPanelSelectionSet SelectedNodes = GraphEditor->GetSelectedNodes();
    GraphEditor->ClearSelectionSet();
    for (UObject* NodeObject : SelectedNodes)
        Cast<UEdGraphNode>(NodeObject)->DestroyNode();
}

void FConcordModelEditorToolkit::DuplicateSelectedNodes()
{
    SerializedNodesToPaste = GetSelectedNodesString();
    PasteNodes(INVTEXT("Duplicate Selected Concord Node(s)"));
}

void FConcordModelEditorToolkit::PasteNodes()
{
    PasteNodes(INVTEXT("Paste Selected Concord Node(s)"));
}

void FConcordModelEditorToolkit::PasteNodes(const FText& TransactionText)
{
    const FScopedTransaction Transaction(TransactionText);
    ConcordModel->Modify();
    ConcordModel->Graph->Modify();

    GraphEditor->ClearSelectionSet();
    TSet<UEdGraphNode*> PastedNodes;
    FEdGraphUtilities::ImportNodesFromText(ConcordModel->Graph, SerializedNodesToPaste, PastedNodes);
    SerializedNodesToPaste.Empty();

    FVector2D Location = GraphEditor->GetPasteLocation(), Offset { 0.0f, 0.0f };
    for (UEdGraphNode* GraphNode : PastedNodes)
        Offset += Location - FVector2D(GraphNode->NodePosX, GraphNode->NodePosY);
    Offset /= PastedNodes.Num();

    for (UEdGraphNode* GraphNode : PastedNodes)
    {
        GraphNode->NodePosX += Offset.X;
        GraphNode->NodePosY += Offset.Y;
        GraphNode->CreateNewGuid();
        GraphEditor->SetNodeSelection(GraphNode, true);
    }

    GraphEditor->NotifyGraphChanged();
}

bool FConcordModelEditorToolkit::CanPasteNodes()
{
    FPlatformApplicationMisc::ClipboardPaste(SerializedNodesToPaste);
    if (FEdGraphUtilities::CanImportNodesFromText(ConcordModel->Graph, SerializedNodesToPaste))
        return true;
    SerializedNodesToPaste.Empty();
    return false;
}

void FConcordModelEditorToolkit::RenameSelectedNode()
{
    const FGraphPanelSelectionSet& SelectedNodes = GraphEditor->GetSelectedNodes();
    for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
        if (const UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter))
            if (Node->GetCanRenameNode())
            {
                GraphEditor->JumpToNode(Node, true);
                return;
            }
}

bool FConcordModelEditorToolkit::CanRenameSelectedNodes() const
{
    const FGraphPanelSelectionSet& SelectedNodes = GraphEditor->GetSelectedNodes();
    for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
        if (const UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter))
            if (Node->GetCanRenameNode())
                return true;
    return false;
}

void FConcordModelEditorToolkit::SampleVariation()
{
    if (!ConcordModel->DefaultSamplerFactory) ConcordModel->DefaultSamplerFactory = NewObject<UConcordExactSamplerFactory>(ConcordModel);
    TOptional<FConcordError> ModelError;
    TSharedPtr<const FConcordFactorGraph<float>> FactorGraph = ConcordModel->GetFactorGraph(ConcordModel->DefaultSamplerFactory->GetCycleMode(), ModelError);
    if (ClearSavedGraphNodeErrors()) GraphEditor->NotifyGraphChanged();
    HandleOptionalError(ModelError);
    if (ModelError) return;
    TOptional<FString> SamplerErrorMessage;
    TSharedPtr<FConcordSampler> Sampler = ConcordModel->DefaultSamplerFactory->CreateSampler(FactorGraph.ToSharedRef(), SamplerErrorMessage);
    if (SamplerErrorMessage) { SetStatusError(SamplerErrorMessage.GetValue()); return; }
    for (const UConcordCrate* Crate : ConcordModel->DefaultCrates) if (Crate) Sampler->GetEnvironment()->SetCrate(Crate->CrateData);
    UConcordModelGraph* Graph = ConcordModelGraph;
    Sampler->GetEnvironment()->SetMaskAndParametersFromStagingArea();
    Sampler->GetVariationFromEnvironment();

    if (Graph->DistributionMode == EConcordModelGraphDistributionMode::Marginal)
    {
        SampleVariationAndInferMarginalsPostSetup(*Sampler.Get(), Graph);
    }
    else
    {
        SetStatusVariationScore(Sampler->SampleVariationSync());

        if (Graph->DistributionMode == EConcordModelGraphDistributionMode::Conditional)
            SetVariationAndProbabilities(*Sampler.Get(), Sampler->GetConditionalProbabilities());
        else
            SetVariationOnly(*Sampler.Get());
    }
    ConcordModel->SetLatestPatternFromSampledVariation(Sampler.Get());
}

void FConcordModelEditorToolkit::HandleOptionalError(const TOptional<FConcordError>& Error)
{
    if (!Error)
    {
        VariationScoreText->SetVisibility(EVisibility::Collapsed);
    }
    else
    {
        SetStatusError(Error.GetValue().Message);
        if (Error.GetValue().Vertex) FocusErrorNode(Error.GetValue().Message, Error.GetValue().Vertex);
    }
}

void FConcordModelEditorToolkit::FocusErrorNode(const FString& ErrorMessage, const UConcordVertex* Vertex)
{
    for (UEdGraphNode* Node : GraphEditor->GetCurrentGraph()->Nodes)
        if (UConcordModelGraphNode* ConcordNode = Cast<UConcordModelGraphNode>(Node))
            if (ConcordNode->Vertex == Vertex)
            {
                ConcordNode->bHasCompilerMessage = true;
                ConcordNode->ErrorType = EMessageSeverity::Error;
                ConcordNode->ErrorMsg = ErrorMessage;
                ConcordNode->OnNodeChanged.Broadcast();
                GraphEditor->JumpToNode(ConcordNode, false, false);
                return;
            }
    if (const UConcordVertex* OuterVertex = Cast<UConcordVertex>(Vertex->GetOuter())) FocusErrorNode(ErrorMessage, OuterVertex);
}

void FConcordModelEditorToolkit::SampleVariationAndInferMarginalsPostSetup(FConcordSampler& Sampler, UConcordModelGraph* Graph)
{
    FConcordProbabilities Marginals;
    SetStatusVariationScore(Sampler.SampleVariationAndInferMarginalsSync(Marginals));
    SetVariationAndProbabilities(Sampler, Marginals);
}

void FConcordModelEditorToolkit::SetVariationAndProbabilities(const FConcordSampler& Sampler, const FConcordProbabilities& Probabilities)
{
    for (UEdGraphNode* Node : ConcordModel->Graph->Nodes)
    {
        if (UConcordModelGraphBox* BoxNode = Cast<UConcordModelGraphBox>(Node))
        {
            const auto& VariationBlock = Sampler.GetFactorGraph()->GetVariationBlocks()[BoxNode->Name];
            BoxNode->OnVariationChanged.Broadcast({ Sampler.GetVariation().GetData() + VariationBlock.Offset, VariationBlock.Size });
            BoxNode->OnDistributionsChanged.Broadcast({ Probabilities.GetData() + VariationBlock.Offset, VariationBlock.Size });
        }
        else if (UConcordModelGraphDistributionViewer* ViewerNode = Cast<UConcordModelGraphDistributionViewer>(Node))
        {
            ViewerNode->OnProbabilitiesChanged.Broadcast(Probabilities, Sampler.GetFactorGraph()->GetVariationBlocks());
        }
        else if (UConcordModelGraphOutput* OutputNode = Cast<UConcordModelGraphOutput>(Node))
        {
            SetOutputNodeValues(OutputNode, Sampler);
        }
    }
}

void FConcordModelEditorToolkit::SetVariationOnly(const FConcordSampler& Sampler)
{
    for (UEdGraphNode* Node : ConcordModel->Graph->Nodes)
    {
        if (UConcordModelGraphBox* BoxNode = Cast<UConcordModelGraphBox>(Node))
        {
            BoxNode->OnNodeChanged.Broadcast(); // ensure entropy ring is reset
            const auto& VariationBlock = Sampler.GetFactorGraph()->GetVariationBlocks()[BoxNode->Name];
            BoxNode->OnVariationChanged.Broadcast({ Sampler.GetVariation().GetData() + VariationBlock.Offset, VariationBlock.Size });
        }
        else if (UConcordModelGraphDistributionViewer* ViewerNode = Cast<UConcordModelGraphDistributionViewer>(Node))
        {
            ViewerNode->OnProbabilitiesChanged.Broadcast({}, {});
        }
        else if (UConcordModelGraphOutput* OutputNode = Cast<UConcordModelGraphOutput>(Node))
        {
            SetOutputNodeValues(OutputNode, Sampler);
        }
    }
}

void FConcordModelEditorToolkit::SetStatusError(const FString& Message)
{
    VariationScoreText->SetVisibility(EVisibility::Visible);
    VariationScoreText->SetColorAndOpacity(FLinearColor::Red);
    VariationScoreText->SetText(FText::FromString(Message));
}

void FConcordModelEditorToolkit::SetStatusVariationScore(float Score)
{
    VariationScoreText->SetVisibility(EVisibility::Visible);
    VariationScoreText->SetColorAndOpacity(FLinearColor::White);
    VariationScoreText->SetText(FText::Format(INVTEXT("Score: {0}"), Score));
}

void FConcordModelEditorToolkit::SetOutputNodeValues(UConcordModelGraphOutput* OutputNode, const FConcordSampler& Sampler) const
{
    const auto& Output = Sampler.GetFactorGraph()->GetOutputs()[OutputNode->Name];
    OutputNode->CurrentOutputValues.Reset();
    switch (Output->GetType())
    {
    case EConcordValueType::Int:
    {
        TArray<int32> Values; Values.SetNumUninitialized(Output->Num());
        Output->Eval(Sampler.GetExpressionContext(), Values);
        for (int32 Value : Values) OutputNode->CurrentOutputValues.Add(Value);
        OutputNode->OnIntOutputValuesChanged.Broadcast(Values);
        break;
    }
    case EConcordValueType::Float:
    {
        TArray<float> Values; Values.SetNumUninitialized(Output->Num());
        Output->Eval(Sampler.GetExpressionContext(), Values);
        for (float Value : Values) OutputNode->CurrentOutputValues.Add(Value);
        OutputNode->OnFloatOutputValuesChanged.Broadcast(Values);
        break;
    }
    default: checkNoEntry();
    }
}

void FConcordModelEditorToolkit::SelectedOutputsToCrate()
{
    auto Crate = Cast<UConcordCrate>(FAssetToolsModule::GetModule().Get().CreateAssetWithDialog(UConcordCrate::StaticClass(), NewObject<UConcordCrateFactoryNew>()));
    if (!Crate) return;
    const FGraphPanelSelectionSet SelectedNodes = GraphEditor->GetSelectedNodes();
    for (UObject* NodeObject : SelectedNodes)
    {
        UConcordModelGraphOutput* OutputNode = Cast<UConcordModelGraphOutput>(NodeObject);
        if (!OutputNode) continue;
        switch (OutputNode->GetOutput()->GetType())
        {
        case EConcordValueType::Int:
        {
            auto& Block = Crate->CrateData.IntBlocks.Add(OutputNode->Name);
            Block.Values.Reserve(OutputNode->CurrentOutputValues.Num());
            for (FConcordValue Value : OutputNode->CurrentOutputValues) Block.Values.Add(Value.Int);
            break;
        }
        case EConcordValueType::Float:
        {
            auto& Block = Crate->CrateData.FloatBlocks.Add(OutputNode->Name);
            Block.Values.Reserve(OutputNode->CurrentOutputValues.Num());
            for (FConcordValue Value : OutputNode->CurrentOutputValues) Block.Values.Add(Value.Float);
            break;
        }
        default: checkNoEntry();
        }
    }
    Crate->PostEditChange();
}

void FConcordModelEditorToolkit::ExportTikz() const
{
    UConcordModelGraphTikzExporter::Export(ConcordModelGraph);
}
