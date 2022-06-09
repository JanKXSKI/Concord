// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "ConcordPatternActions.h"
#include "ConcordPattern.h"
#include "ConcordCrate.h"
#include "AssetToolsModule.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

FText FConcordPatternActions::GetName() const
{
    return INVTEXT("Concord Pattern");
}

uint32 FConcordPatternActions::GetCategories()
{
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    return AssetTools.FindAdvancedAssetCategory(FName(TEXT("Concord")));
}

FColor FConcordPatternActions::GetTypeColor() const
{
	return FColor::Purple;
}

FText FConcordPatternActions::GetAssetDescription(const FAssetData &AssetData) const
{
    return INVTEXT("A Concord pattern containing note columns for playback as in a tracker.");
}

UClass* FConcordPatternActions::GetSupportedClass() const
{
    return UConcordPattern::StaticClass();
}

bool FConcordPatternActions::IsImportedAsset() const
{
    return true;
}

void FConcordPatternActions::GetResolvedSourceFilePaths(const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths) const
{
    for (UObject* Asset : TypeAssets)
    {
        const UConcordPattern* Pattern = Cast<UConcordPattern>(Asset);
        if (!Pattern || !Pattern->AssetImportData) continue;
        OutSourceFilePaths.Add(Pattern->AssetImportData->GetFirstFilename());
    }
}

bool FConcordPatternActions::HasActions(const TArray<UObject*>& InObjects) const
{
    return true;
}

void FConcordPatternActions::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder)
{
	auto Patterns = GetTypedWeakObjectPtrs<UConcordPattern>(InObjects);

	auto ExecuteAction = FExecuteAction::CreateLambda([=]
	{
		for (const auto& Pattern : Patterns)
		{
			if (!Pattern.IsValid()) continue;
			const FString CrateName = Pattern->GetName() + TEXT("Crate");
			const FString CratePackagePath = FPaths::GetPath(Pattern->GetPackage()->GetName());
			auto Crate = Cast<UConcordCrate>(FAssetToolsModule::GetModule().Get().CreateAsset(CrateName, CratePackagePath, UConcordCrate::StaticClass(), nullptr));
			if (!Crate) continue;
			Crate->CrateData = Pattern->GetCrate();
			Crate->PostEditChange();
		}
	});

	auto CanExecuteAction = FCanExecuteAction::CreateLambda([=]
	{
		for (auto& Pattern : Patterns) if (Pattern.IsValid()) return true;
		return false;
	});

	MenuBuilder.AddMenuEntry(INVTEXT("Create Crate From Pattern"),
							 INVTEXT("Converts each column of each track in the pattern into an IntBlock in a Crate."),
							 FSlateIcon(),
							 FUIAction(ExecuteAction, CanExecuteAction));
}
