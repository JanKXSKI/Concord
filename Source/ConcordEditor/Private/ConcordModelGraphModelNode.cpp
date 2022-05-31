// Copyright Jan Klimaschewski. All Rights Reserved.

#include "ConcordModelGraphModelNode.h"
#include "SConcordModelGraphInOutNode.h"
#include "ConcordModel.h"
#include "ConcordParameter.h"
#include "ConcordOutput.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetData.h"
#include "FileHelpers.h"

FText UConcordModelGraphModelNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    switch (TitleType)
    {
    case ENodeTitleType::FullTitle: return FText::Format(INVTEXT("{0}\n{1}"), FText::FromName(Name), Vertex->GetDisplayName());
    default: return FText::FromName(Name);
    }
}

TSharedPtr<SGraphNode> UConcordModelGraphModelNode::CreateVisualWidget()
{
    PreCreateVisualWidget();
    return SNew(SConcordModelGraphInOutNode, this)
    .bWithDefaultValues(false)
    .bWithOutputLabels(true)
    .bTitleOnTop(true);
}

void UConcordModelGraphModelNode::AllocateDefaultPins()
{
    SyncPins();
}

void UConcordModelGraphModelNode::PostEditImport()
{
    UConcordModelGraphConsumer::PostEditImport();
    AddInputOutputChangedDelegate();
}

void UConcordModelGraphModelNode::PostDuplicate(bool bDuplicateForPie)
{
    UConcordModelGraphConsumer::PostDuplicate(bDuplicateForPie);
    AddInputOutputChangedDelegate();
}

void UConcordModelGraphModelNode::AddInputOutputChangedDelegate()
{
    UConcordModelBase* Model = GetReferencedModel();
    if (!Model) return;
    Model->Modify();
    Model->InputOutputInterfaceListeners.AddUnique({ this, "SyncPins" });
    SavePackage(Model->GetPackage());
}

void UConcordModelGraphModelNode::SyncPins()
{
    UConcordModelBase* Model = GetReferencedModel();
    Modify();
    Vertex->Modify();
    PreSyncPins();
    if (!Model)
    {
        while (!Pins.IsEmpty())
            RemovePin(Pins.Last());
    }
    else
    {
        TMap<FName, UEdGraphPin*> ExistingPins;
        TArray<UEdGraphPin*> PinsToRemove;
        for (UEdGraphPin* Pin : Pins)
            if (Pin->Direction == EGPD_Input)
            {
                if (Vertex->GetInputInfo().ContainsByPredicate([Pin](const auto& InputInfo) { return Pin->GetDisplayName().EqualToCaseIgnored(InputInfo.DisplayName); }))
                    ExistingPins.Add(Pin->GetFName(), Pin);
                else
                    PinsToRemove.Add(Pin);
            }
            else
            {
                if (Model->OrderedOutputNames.Contains(Pin->GetFName()))
                    ExistingPins.Add(Pin->GetFName(), Pin);
                else
                    PinsToRemove.Add(Pin);
            }
        for (UEdGraphPin* Pin : PinsToRemove) RemovePin(Pin);
        Pins.Reset();

        for (const auto& InputInfo : Vertex->GetInputInfo())
        {
            const FName InputName = FName(InputInfo.DisplayName.ToString());
            if (UEdGraphPin** ExistingPin = ExistingPins.Find(InputName))
                Pins.Add(*ExistingPin);
            else
                CreatePin(EGPD_Input, "Expression", InputName);
        }
        for (const FName& OutputName : Model->OrderedOutputNames)
        {
            if (UEdGraphPin** ExistingPin = ExistingPins.Find(OutputName))
                Pins.Add(*ExistingPin);
            else
                CreatePin(EGPD_Output, "Expression", OutputName);
        }
    }

    Vertex->ConnectedTransformers.SetNum(Vertex->GetInputInfo().Num());
    SyncModelConnections();
    OnNodeChanged.Broadcast();
    SavePackage(GetPackage());
}

void UConcordModelGraphModelNode::DestroyNode()
{
    UConcordModelBase* Model = GetReferencedModel();
    if (Model)
    {
        Model->Modify();
        Model->InputOutputInterfaceListeners.Remove({ this, "SyncPins" });
        SavePackage(Model->GetPackage());
    }
    UEdGraphNode::DestroyNode();
}

void UConcordModelGraphComposite::GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
    if (ContextMenuBuilder.FromPin &&
        ContextMenuBuilder.FromPin->PinType.PinCategory == "Viewer") return;

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> AssetDatas;
    AssetRegistryModule.Get().GetAssetsByClass(UConcordModel::StaticClass()->GetFName(), AssetDatas);
    for (const FAssetData& AssetData : AssetDatas)
        if (AssetData.ObjectPath != FName(Cast<UConcordModelGraph>(ContextMenuBuilder.CurrentGraph)->GetModel()->GetPathName()))
            ContextMenuBuilder.AddAction(MakeShared<FConcordModelGraphAddNodeAction_NewComposite>(AssetData));
}

void UConcordModelGraphComposite::DestroyNode()
{
    GetModel()->Modify();
    GetModel()->Composites.Remove(Name);
    GetModel()->Composites.Shrink();
    UConcordModelGraphModelNode::DestroyNode();
}

FConcordModelGraphAddNodeAction_NewComposite::FConcordModelGraphAddNodeAction_NewComposite()
    : FConcordModelGraphAddNodeAction_NewComposite(FAssetData())
{}

FConcordModelGraphAddNodeAction_NewComposite::FConcordModelGraphAddNodeAction_NewComposite(FAssetData InModelAssetData)
    : FConcordModelGraphAddNodeAction(INVTEXT("Composites"),
                                      FText::FromName(InModelAssetData.AssetName),
                                      Cast<UConcordVertex>(UConcordComposite::StaticClass()->GetDefaultObject())->GetTooltip(),
                                      INVTEXT("Add New Concord Composite"))
    , ModelAssetData(InModelAssetData)
{}

UEdGraphNode* FConcordModelGraphAddNodeAction_NewComposite::MakeNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin)
{
    FGraphNodeCreator<UConcordModelGraphComposite> NodeCreator(*ParentGraph);
    UConcordModelGraphComposite* NewGraphComposite = NodeCreator.CreateUserInvokedNode();
    NewGraphComposite->Model = Cast<UConcordModel>(ModelAssetData.GetAsset());
    NewGraphComposite->Name = "NewComposite";
    NewGraphComposite->Vertex = NewObject<UConcordComposite>(NewGraphComposite->GetModel(), NAME_None, RF_Transactional);
    NewGraphComposite->SyncModelName(false);
    NewGraphComposite->AddInputOutputChangedDelegate();
    NewGraphComposite->GetComposite()->Model = NewGraphComposite->Model;
    NodeCreator.Finalize();
    return NewGraphComposite;
}

void UConcordModelGraphInstance::GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
    if (ContextMenuBuilder.FromPin &&
        ContextMenuBuilder.FromPin->PinType.PinCategory == "Viewer") return;

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> AssetDatas;
    AssetRegistryModule.Get().GetAssetsByClass(UConcordModelBase::StaticClass()->GetFName(), AssetDatas, true);
    for (const FAssetData& AssetData : AssetDatas)
    {
        if (AssetData.ObjectPath == FName(Cast<UConcordModelGraph>(ContextMenuBuilder.CurrentGraph)->GetModel()->GetPathName())) continue;
        const UConcordModel* ModelAsset = Cast<UConcordModel>(AssetData.GetAsset());
        if (ModelAsset && ModelAsset->Boxes.IsEmpty()) continue; // models without boxes should be added as composites, not instances
        ContextMenuBuilder.AddAction(MakeShared<FConcordModelGraphAddNodeAction_NewInstance>(AssetData));
    }
}

void UConcordModelGraphInstance::DestroyNode()
{
    GetModel()->Modify();
    GetModel()->Instances.Remove(Name);
    GetModel()->Instances.Shrink();
    UConcordModelGraphModelNode::DestroyNode();
}

void UConcordModelGraphInstance::PreCreateVisualWidget()
{
    GetInstance()->UpdateCachedInputOutputInfo();
}

void UConcordModelGraphInstance::PreSyncPins()
{
    GetInstance()->UpdateCachedInputOutputInfo();
}

FConcordModelGraphAddNodeAction_NewInstance::FConcordModelGraphAddNodeAction_NewInstance()
    : FConcordModelGraphAddNodeAction_NewInstance(FAssetData())
{}

FConcordModelGraphAddNodeAction_NewInstance::FConcordModelGraphAddNodeAction_NewInstance(FAssetData InModelAssetData)
    : FConcordModelGraphAddNodeAction(INVTEXT("Instances"),
                                      FText::FromName(InModelAssetData.AssetName),
                                      Cast<UConcordVertex>(UConcordInstance::StaticClass()->GetDefaultObject())->GetTooltip(),
                                      INVTEXT("Add New Concord Instance"))
    , ModelAssetData(InModelAssetData)
{}

UEdGraphNode* FConcordModelGraphAddNodeAction_NewInstance::MakeNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin)
{
    FGraphNodeCreator<UConcordModelGraphInstance> NodeCreator(*ParentGraph);
    UConcordModelGraphInstance* NewGraphInstance = NodeCreator.CreateUserInvokedNode();
    NewGraphInstance->Model = Cast<UConcordModelBase>(ModelAssetData.GetAsset());
    NewGraphInstance->Name = "NewInstance";
    NewGraphInstance->Vertex = NewObject<UConcordInstance>(NewGraphInstance->GetModel(), NAME_None, RF_Transactional);
    NewGraphInstance->SyncModelName(false);
    NewGraphInstance->AddInputOutputChangedDelegate();
    NewGraphInstance->GetInstance()->Model = NewGraphInstance->Model;
    NodeCreator.Finalize();
    return NewGraphInstance;
}
