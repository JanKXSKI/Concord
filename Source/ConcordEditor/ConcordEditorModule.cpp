// Copyright Jan Klimaschewski. All Rights Reserved.

#include "CoreMinimal.h"
#include "ConcordModelActions.h"
#include "ConcordNativeModelActions.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Interfaces/IPluginManager.h"

class FConcordEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
        AssetTools.RegisterAdvancedAssetCategory("Concord", INVTEXT("Concord"));
        ConcordModelActions = MakeShareable(new FConcordModelActions);
        AssetTools.RegisterAssetTypeActions(ConcordModelActions.ToSharedRef());
        ConcordNativeModelActions = MakeShareable(new FConcordNativeModelActions);
        AssetTools.RegisterAssetTypeActions(ConcordNativeModelActions.ToSharedRef());

        MakeStyle();
    }

    virtual void ShutdownModule() override
    {
        if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
        {
            IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
            AssetTools.UnregisterAssetTypeActions(ConcordModelActions.ToSharedRef());
            AssetTools.UnregisterAssetTypeActions(ConcordNativeModelActions.ToSharedRef());
        }
    }
private:
    void MakeStyle()
    {
        Style = MakeShareable(new FSlateStyleSet("ConcordEditorStyle"));
        Style->SetContentRoot(IPluginManager::Get().FindPlugin("Concord")->GetContentDir() / TEXT("Editor/Slate"));

        Style->Set("ClassIcon.ConcordModel", new FSlateImageBrush(Style->RootToContentDir(TEXT("Icon_128x.png")), FVector2D(16.0f, 16.0f)));
        Style->Set("ClassThumbnail.ConcordModel", new FSlateImageBrush(Style->RootToContentDir(TEXT("Icon_128x.png")), FVector2D(64.0f, 64.0f)));

        Style->Set("ClassIcon.ConcordCrate", new FSlateImageBrush(Style->RootToContentDir(TEXT("CrateIcon_128x.png")), FVector2D(16.0f, 16.0f)));
        Style->Set("ClassThumbnail.ConcordCrate", new FSlateImageBrush(Style->RootToContentDir(TEXT("CrateIcon_128x.png")), FVector2D(64.0f, 64.0f)));

        Style->Set("ClassIcon.ConcordPattern", new FSlateImageBrush(Style->RootToContentDir(TEXT("PatternIcon_128x.png")), FVector2D(16.0f, 16.0f)));
        Style->Set("ClassThumbnail.ConcordPattern", new FSlateImageBrush(Style->RootToContentDir(TEXT("PatternIcon_128x.png")), FVector2D(64.0f, 64.0f)));

        Style->Set("ClassIcon.ConcordTrackerModule", new FSlateImageBrush(Style->RootToContentDir(TEXT("ModuleIcon_128x.png")), FVector2D(16.0f, 16.0f)));
        Style->Set("ClassThumbnail.ConcordTrackerModule", new FSlateImageBrush(Style->RootToContentDir(TEXT("ModuleIcon_128x.png")), FVector2D(64.0f, 64.0f)));

        Style->Set("ClassIcon.ConcordNativeModel", new FSlateImageBrush(Style->RootToContentDir(TEXT("NativeIcon_128x.png")), FVector2D(16.0f, 16.0f)));
        Style->Set("ClassThumbnail.ConcordNativeModel", new FSlateImageBrush(Style->RootToContentDir(TEXT("NativeIcon_128x.png")), FVector2D(64.0f, 64.0f)));

        Style->Set("PinBackgroundHovered", new FSlateColorBrush(FColor(0, 0, 0, 100)));
        Style->Set("EntropyPinBack", new FSlateImageBrush(Style->RootToContentDir(TEXT("EntropyPinBack_20x20.png")), FVector2D(20.0f, 20.0f)));
        Style->Set("EntropyPinFront", new FSlateImageBrush(Style->RootToContentDir(TEXT("EntropyPinFront_20x20.png")), FVector2D(20.0f, 20.0f)));
        Style->Set("EntropyPinFrontDisconnected", new FSlateImageBrush(Style->RootToContentDir(TEXT("EntropyPinFrontDisconnected_20x20.png")), FVector2D(20.0f, 20.0f)));
        Style->Set("DistributionViewerBackground", new FSlateImageBrush(Style->RootToContentDir(TEXT("DistributionViewerBackground_200x.png")), FVector2D(200.0f, 200.0f)));
        Style->Set("DistributionViewerScreen", new FSlateImageBrush(Style->RootToContentDir(TEXT("DistributionViewerScreen_200x.png")), FVector2D(200.0f, 200.0f)));
        FScrollBarStyle ScrollBarStyle = FCoreStyle::Get().GetWidgetStyle<FScrollBarStyle>("Scrollbar");
        ScrollBarStyle.SetNormalThumbImage(FSlateColorBrush(FColor(23, 23, 23, 255)));
        ScrollBarStyle.SetHoveredThumbImage(FSlateColorBrush(FColor(64, 64, 64, 255)));
        ScrollBarStyle.SetDraggedThumbImage(FSlateColorBrush(FColor(96, 96, 96, 255)));
        Style->Set("DistributionViewerScrollBar", ScrollBarStyle);
        Style->Set("ConcordModelEditor.SampleVariation", new FSlateImageBrush(Style->RootToContentDir(TEXT("SampleVariation_40x.png")), FVector2D(40.0f, 40.0f)));
        Style->Set("ConcordModelEditor.SampleVariation.Small", new FSlateImageBrush(Style->RootToContentDir(TEXT("SampleVariation_40x.png")), FVector2D(20.0f, 20.0f)));
        Style->Set("ConcordModelEditor.SelectedOutputsToCrate", new FSlateImageBrush(Style->RootToContentDir(TEXT("OutputCrate_40x.png")), FVector2D(40.0f, 40.0f)));
        Style->Set("ConcordModelEditor.SelectedOutputsToCrate.Small", new FSlateImageBrush(Style->RootToContentDir(TEXT("OutputCrate_40x.png")), FVector2D(20.0f, 20.0f)));
        Style->Set("ObservedInstanceNodeBody", new FSlateBoxBrush(Style->RootToContentDir(TEXT("ObservedInstanceNodeBody_64x64.png")), FMargin(16.f/64.f, 25.f/64.f, 16.f/64.f, 16.f/64.f)));

        FInlineEditableTextBlockStyle InOutNodeTitleStyle = FCoreStyle::Get().GetWidgetStyle<FInlineEditableTextBlockStyle>("InlineEditableTextBlockStyle");
        InOutNodeTitleStyle.TextStyle.SetFontSize(11);
        InOutNodeTitleStyle.TextStyle.SetTypefaceFontName("Bold");
        InOutNodeTitleStyle.TextStyle.SetColorAndOpacity(FLinearColor::Gray);
        Style->Set("InOutNodeTitle", InOutNodeTitleStyle);
        InOutNodeTitleStyle.TextStyle.SetFontSize(16);
        Style->Set("InOutNodeTitleSymbol", InOutNodeTitleStyle);

        FSlateStyleRegistry::RegisterSlateStyle(*Style);
    }

    TSharedPtr<FConcordModelActions> ConcordModelActions;
    TSharedPtr<FConcordNativeModelActions> ConcordNativeModelActions;
    TSharedPtr<FSlateStyleSet> Style;
};

IMPLEMENT_MODULE(FConcordEditorModule, ConcordEditor);
