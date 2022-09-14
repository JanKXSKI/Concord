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
#include "ConcordVertex.h"

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
    , DistanceScale(0.0f)
{}

void UConcordModelGraphTikzExporter::ExportInteral(const UConcordModelGraph* Graph)
{
    Buffer.Empty(1 << 15);
    Center = GetCenter(Graph);
    WriteHeader();
    for (const UEdGraphNode* Node : Graph->Nodes) WriteNode(Node);
    for (const UEdGraphNode* Node : Graph->Nodes) WriteOutgoingEdges(Node);
    WriteFooter();
    FFileHelper::SaveStringToFile(Buffer, *(Directory.Path / FileName), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

FVector2D UConcordModelGraphTikzExporter::GetCenter(const UEdGraph* Graph)
{
    FVector2D Center = { 0.0, 0.0 };
    for (const UEdGraphNode* Node : Graph->Nodes)
    {
        Center.X += Node->NodePosX;
        Center.Y += Node->NodePosY;
    }
    Center /= Graph->Nodes.Num();
    return Center;
}

void UConcordModelGraphTikzExporter::WriteHeader()
{
    Buffer += TEXT(R"_(\usetikzlibrary{fit, positioning}
\begin{tikzpicture}[every matrix/.style={column sep = 0.2cm}]
    \tikzstyle{pin} = [above = -0.09cm, fill, circle, minimum size = 0.2cm, inner sep = 0cm]
    \tikzstyle{pinlabel} = [font = \footnotesize, minimum width = 0.5cm]
    \tikzstyle{inlabel} = [pinlabel, right = 0.1cm, anchor = mid west]
    \tikzstyle{outlabel} = [pinlabel, left = 0.1cm, anchor = mid east]
    \tikzstyle{nodetitle} = [minimum height = 0.7cm]
    \tikzstyle{nodedetails} = [font = \footnotesize, text = gray]
)_");
}

void UConcordModelGraphTikzExporter::WriteNode(const UEdGraphNode* Node)
{
    double X = (Node->NodePosX - Center.X) * 0.02 * FMath::Pow(2.0, DistanceScale);
    double Y = (Center.Y - Node->NodePosY) * 0.02 * FMath::Pow(2.0, DistanceScale);
    Buffer += TEXT("    \\matrix(");
    WriteID(Node, TEXT("matrix"));
    Buffer += TEXT(") at ");
    Buffer += FString::Printf(TEXT("(%.3f, %.3f)"), X, Y);
    Buffer += TEXT("\n    {\n");
    WriteInputOutputPinPairs(Node);
    Buffer += TEXT("    };\n");

    Buffer += TEXT("    \\node[nodetitle, above = 0cm of ");
    WriteID(Node, TEXT("matrix"));
    Buffer += TEXT(".north](");
    WriteID(Node, TEXT("title"));
    Buffer += TEXT("){");
    WriteTitle(Node);
    Buffer += TEXT("};\n");

    Buffer += TEXT("    \\matrix[below = 0cm of ");
    WriteID(Node, TEXT("matrix"));
    Buffer += TEXT(".south](");
    WriteID(Node, TEXT("details"));
    Buffer += TEXT(")\n    {\n");
    WriteDetails(Node);
    Buffer += TEXT("    };\n");

    Buffer += TEXT("    \\node[draw, rounded corners, fit = (");
    WriteID(Node, TEXT("matrix"));
    Buffer += TEXT(") (");
    WriteID(Node, TEXT("title"));
    Buffer += TEXT(")] {};\n");

    WriteDefaultValues(Node);
}

void UConcordModelGraphTikzExporter::WriteOutgoingEdges(const UEdGraphNode* Node)
{
    for (const UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin->Direction == EGPD_Input) continue;
        for (const UEdGraphPin* ConnectedPin : Pin->LinkedTo)
        {
            Buffer += TEXT("    \\draw (");
            WritePinID(Pin);
            Buffer += TEXT(") to [out = 0, in = 180] (");
            WritePinID(ConnectedPin);
            Buffer += TEXT(");\n");
        }
    }
}

void UConcordModelGraphTikzExporter::WriteDefaultValues(const UEdGraphNode* Node)
{
    for (const UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin->Direction == EGPD_Output) continue;
        if (!Pin->LinkedTo.IsEmpty()) continue;
        const FString DefaultString = Pin->GetDefaultAsString();
        if (DefaultString.IsEmpty()) continue;
        Buffer += TEXT("    \\node [left = 0.5cm of ");
        WritePinID(Pin);
        Buffer += TEXT("] (");
        WritePinID(Pin, TEXT("default"));
        Buffer += TEXT(") {");
        Buffer += DefaultString;
        Buffer += TEXT("};\n");

        Buffer += TEXT("    \\draw (");
        WritePinID(Pin);
        Buffer += TEXT(") -- (");
        WritePinID(Pin, TEXT("default"));
        Buffer += TEXT(");\n");
    }
}

void UConcordModelGraphTikzExporter::WriteFooter()
{
    Buffer += TEXT("\\end{tikzpicture}");
}

void UConcordModelGraphTikzExporter::WriteID(const UEdGraphNode* Node, const TCHAR* Suffix, TOptional<EEdGraphPinDirection> PinDirection)
{
    Buffer += Node->GetName();
    if (PinDirection)
    {
        if (PinDirection.GetValue() == EGPD_Input) Buffer += TEXT("__TIKZ_INPIN__");
        else Buffer += TEXT("__TIKZ_OUTPIN__");
    }
    else Buffer += TEXT("__TIKZ__");
    Buffer += Suffix;
}

void UConcordModelGraphTikzExporter::WritePinID(const UEdGraphPin* Pin, const TCHAR* Suffix)
{
    int32 PinIndex = 0;
    Pin->GetOwningNode()->Pins.Find(const_cast<UEdGraphPin*>(Pin), PinIndex);
    WriteID(Pin->GetOwningNode(), *FString::Printf(TEXT("%d"), PinIndex), Pin->Direction.GetValue());
    Buffer += Suffix;
}

void UConcordModelGraphTikzExporter::WriteTitle(const UEdGraphNode* Node)
{
    Buffer += Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
}

void UConcordModelGraphTikzExporter::WriteInputOutputPinPairs(const UEdGraphNode* Node, int32 InputPinIndex, int32 OutputPinIndex)
{
    while (InputPinIndex < Node->Pins.Num() && Node->Pins[InputPinIndex]->Direction == EGPD_Output) ++InputPinIndex;
    while (OutputPinIndex < Node->Pins.Num() && Node->Pins[OutputPinIndex]->Direction == EGPD_Input) ++OutputPinIndex;
    if (InputPinIndex >= Node->Pins.Num() && OutputPinIndex >= Node->Pins.Num()) return;
    Buffer += TEXT("        ");
    if (InputPinIndex < Node->Pins.Num()) WritePin(Node->Pins[InputPinIndex++]);
    else WritePinPlaceholder(EGPD_Input);
    Buffer += TEXT(" & ");
    if (OutputPinIndex < Node->Pins.Num()) WritePin(Node->Pins[OutputPinIndex++]);
    else WritePinPlaceholder(EGPD_Output);
    Buffer += TEXT(" \\\\\n");
    WriteInputOutputPinPairs(Node, InputPinIndex, OutputPinIndex);
}

void UConcordModelGraphTikzExporter::WritePin(const UEdGraphPin* Pin)
{
    Buffer += TEXT("\\node [pin] (");
    WritePinID(Pin);
    Buffer += TEXT(") {}; \\node [");
    if (Pin->Direction == EGPD_Input) Buffer += TEXT("inlabel");
    else Buffer += TEXT("outlabel");
    Buffer += TEXT("] {");
    if (!Pin->Direction == EGPD_Output || !Pin->GetOwningNode()->IsA<UConcordModelGraphTransformer>())
        Buffer += Pin->GetOwningNode()->GetPinDisplayName(Pin).ToString();
    Buffer += TEXT("};");
}

void UConcordModelGraphTikzExporter::WritePinPlaceholder(EEdGraphPinDirection Direction)
{
    Buffer += TEXT("\\node [");
    if (Direction == EGPD_Input) Buffer += TEXT("inlabel");
    else Buffer += TEXT("outlabel");
    Buffer += TEXT("] {}; ");
}

void UConcordModelGraphTikzExporter::WriteDetails(const UEdGraphNode* Node)
{
    Buffer += TEXT("        \\node [nodedetails] {Shape: ");
    Buffer += ConcordShape::ToString(Cast<UConcordModelGraphNode>(Node)->Vertex->GetShape());
    Buffer += TEXT("}; \\\\\n");
}
