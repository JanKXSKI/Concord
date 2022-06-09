// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "ConcordModelActions.h"
#include "ConcordModel.h"
#include "ConcordModelEditorToolkit.h"

FText FConcordModelActions::GetName() const
{
    return INVTEXT("Concord Model");
}

uint32 FConcordModelActions::GetCategories()
{
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    return AssetTools.FindAdvancedAssetCategory(FName(TEXT("Concord")));
}

FColor FConcordModelActions::GetTypeColor() const
{
     return FColor::Purple;
}

FText FConcordModelActions::GetAssetDescription(const FAssetData &AssetData) const
{
    return INVTEXT("A Concord model is a graphical represenation of an adaptive distribution over discrete random variables, e.g. musical symbols.");
}

UClass* FConcordModelActions::GetSupportedClass() const
{
    return UConcordModel::StaticClass();
}

void FConcordModelActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
    for (UObject* Object : InObjects)
    {
        UConcordModel* ConcordModel = Cast<UConcordModel>(Object);
        if (!ConcordModel) continue;
        TSharedRef<FConcordModelEditorToolkit> EditorToolkit = MakeShareable(new FConcordModelEditorToolkit);
        EditorToolkit->Initialize(ConcordModel);
    }
}
