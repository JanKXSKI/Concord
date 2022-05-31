// Copyright Jan Klimaschewski. All Rights Reserved.

#include "CoreMinimal.h"
#include "ConcordPatternActions.h"
#include "ConcordPatternCustomization.h"
#include "ConcordCrateActions.h"
#include "ConcordTrackerModuleActions.h"
#include "PropertyEditorModule.h"
#include "PropertyEditorDelegates.h"

class FConcordSystemEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
        ConcordPatternActions = MakeShareable(new FConcordPatternActions);
        AssetTools.RegisterAssetTypeActions(ConcordPatternActions.ToSharedRef());
        ConcordCrateActions = MakeShareable(new FConcordCrateActions);
        AssetTools.RegisterAssetTypeActions(ConcordCrateActions.ToSharedRef());
        ConcordTrackerModuleActions = MakeShareable(new FConcordTrackerModuleActions);
        AssetTools.RegisterAssetTypeActions(ConcordTrackerModuleActions.ToSharedRef());

        FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
        PropertyModule.RegisterCustomClassLayout("ConcordPattern", FOnGetDetailCustomizationInstance::CreateStatic(&FConcordPatternCustomization::MakeInstance));
        PropertyModule.RegisterCustomClassLayout("ConcordModelBase", FOnGetDetailCustomizationInstance::CreateStatic(&FConcordPatternCustomization::MakeInstance));
    }

    virtual void ShutdownModule() override
    {
        if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
        {
            IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
            AssetTools.UnregisterAssetTypeActions(ConcordPatternActions.ToSharedRef());
            AssetTools.UnregisterAssetTypeActions(ConcordCrateActions.ToSharedRef());
            AssetTools.UnregisterAssetTypeActions(ConcordTrackerModuleActions.ToSharedRef());
        }
    }
private:
    TSharedPtr<FConcordPatternActions> ConcordPatternActions;
    TSharedPtr<FConcordCrateActions> ConcordCrateActions;
    TSharedPtr<FConcordTrackerModuleActions> ConcordTrackerModuleActions;
};

IMPLEMENT_MODULE(FConcordSystemEditorModule, ConcordSystemEditor);
