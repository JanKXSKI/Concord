// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "ConcordModelGraphTikzExporter.h"
#include "ConcordModelGraph.h"
#include "Widgets/SWindow.h"
#include "Interfaces/IMainFrameModule.h"
#include "Modules/ModuleManager.h"
#include "Application/SlateApplicationBase.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformApplicationMisc.h"
#include "PropertyEditorModule.h"
#include "Widgets/Input/SButton.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformProcess.h"

void UConcordModelGraphTikzExporter::Export(const UConcordModelGraph* Graph)
{
	UConcordModelGraphTikzExporter* Exporter = FindObjectFast<UConcordModelGraphTikzExporter>(GetTransientPackage(), "ConcordModelGraphTikzExporter");
    if (!Exporter)
    {
        Exporter = NewObject<UConcordModelGraphTikzExporter>(GetTransientPackage(), "ConcordModelGraphTikzExporter");
        Exporter->AddToRoot();
    }

    TSharedPtr<SWindow> ParentWindow;
    if(FModuleManager::Get().IsModuleLoaded("MainFrame"))
    {
        IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
        ParentWindow = MainFrame.GetParentWindow();
    }
    FVector2D WindowSize { 750.0f, 250.0f };
    FSlateRect WorkAreaRect = FSlateApplicationBase::Get().GetPreferredWorkArea();
    const FVector2D DisplayTopLeft(WorkAreaRect.Left, WorkAreaRect.Top);
    const FVector2D DisplaySize(WorkAreaRect.Right - WorkAreaRect.Left, WorkAreaRect.Bottom - WorkAreaRect.Top);
    const float ScaleFactor = FPlatformApplicationMisc::GetDPIScaleFactorAtPoint(DisplayTopLeft.X, DisplayTopLeft.Y);
    WindowSize *= ScaleFactor;
    const FVector2D WindowPosition = (DisplayTopLeft + (DisplaySize - WindowSize) / 2.0f) / ScaleFactor;

    TSharedRef<SWindow> Window = SNew(SWindow)
    .Title(INVTEXT("Export graph as .tikz"))
    .SizingRule(ESizingRule::UserSized)
    .AutoCenter(EAutoCenter::None)
    .ClientSize(WindowSize)
    .ScreenPosition(WindowPosition);

    FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    FDetailsViewArgs Args;
    Args.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Hide;
    Args.NameAreaSettings = FDetailsViewArgs::HideNameArea;
    TSharedRef<IDetailsView> ExporterDetailsView = PropertyModule.CreateDetailView(Args);
    ExporterDetailsView->HideFilterArea(true);
    ExporterDetailsView->SetObject(Exporter);

    TSharedRef<SWidget> WindowContent = SNew(SVerticalBox)
    +SVerticalBox::Slot()
    .AutoHeight()
    [ ExporterDetailsView ]
    +SVerticalBox::Slot()
    .VAlign(VAlign_Bottom)
    .HAlign(HAlign_Center)
    .Padding(0.0f, 4.0f)
    [
        SNew(SButton)
        .VAlign(VAlign_Center)
        .Text(INVTEXT("Export"))
        .OnClicked_Lambda([Window]{ Window->RequestDestroyWindow(); return FReply::Handled(); })
    ];
    Window->SetContent(WindowContent);

    FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);

	Exporter->ExportInteral(Graph);
}

UConcordModelGraphTikzExporter::UConcordModelGraphTikzExporter()
	: Directory({ FPlatformProcess::UserDir() })
    , FileName("Graph.tikz")
	, DistanceScale(1.0f)
{}

void UConcordModelGraphTikzExporter::ExportInteral(const UConcordModelGraph* Graph)
{
	FFileHelper::SaveStringToFile(FString("FooBar"), *(Directory.Path / FileName));
	// TODO
}
