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
    , bHideAdvancedPins(false)
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
    \tikzstyle{nodetitle} = [font = \small, minimum height = 0.6cm]
    \tikzstyle{nodedetails} = [font = \footnotesize, text = gray]
    \tikzstyle{nodebordertransformer} = [draw, rounded corners]
    \tikzstyle{nodeborder} = [nodebordertransformer, double, double distance = 0.08cm, thick]
)_");
}

void UConcordModelGraphTikzExporter::WriteNode(const UEdGraphNode* Node)
{
    double X = (Node->NodePosX - Center.X) * 0.0175 * FMath::Pow(2.0, DistanceScale);
    double Y = (Center.Y - Node->NodePosY) * 0.0175 * FMath::Pow(2.0, DistanceScale);
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

    Buffer += TEXT("    \\node[");
    if (Node->IsA<UConcordModelGraphTransformer>()) Buffer += TEXT("nodebordertransformer");
    else Buffer += TEXT("nodeborder");
    Buffer += TEXT(", fit = (");
    WriteID(Node, TEXT("matrix"));
    Buffer += TEXT(") (");
    WriteID(Node, TEXT("title"));
    Buffer += TEXT(")](");
    WriteID(Node, TEXT("border"));
    Buffer += TEXT("){};\n");

    Buffer += TEXT("    \\matrix[below = 0cm of ");
    WriteID(Node, TEXT("border"));
    Buffer += TEXT(".south]\n    {\n");
    WriteDetails(Node);
    Buffer += TEXT("    };\n");

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
    Buffer += Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
}

void UConcordModelGraphTikzExporter::WriteInputOutputPinPairs(const UEdGraphNode* Node, int32 InputPinIndex, int32 OutputPinIndex)
{
    while (InputPinIndex < Node->Pins.Num() && Node->Pins[InputPinIndex]->Direction == EGPD_Output) ++InputPinIndex;
    while (OutputPinIndex < Node->Pins.Num() && Node->Pins[OutputPinIndex]->Direction == EGPD_Input) ++OutputPinIndex;
    if (InputPinIndex >= Node->Pins.Num() && OutputPinIndex >= Node->Pins.Num()) return;
    if ((InputPinIndex >= Node->Pins.Num() || !ShouldWritePin(Node->Pins[InputPinIndex])) &&
        (OutputPinIndex >= Node->Pins.Num() || !ShouldWritePin(Node->Pins[OutputPinIndex])))
    {
        WriteInputOutputPinPairs(Node, InputPinIndex + 1, OutputPinIndex + 1);
        return;
    }

    Buffer += TEXT("        ");
    if (InputPinIndex < Node->Pins.Num() && ShouldWritePin(Node->Pins[InputPinIndex])) WritePin(Node->Pins[InputPinIndex]);
    else WritePinPlaceholder(EGPD_Input);
    Buffer += TEXT(" & ");
    if (OutputPinIndex < Node->Pins.Num() && ShouldWritePin(Node->Pins[OutputPinIndex])) WritePin(Node->Pins[OutputPinIndex]);
    else WritePinPlaceholder(EGPD_Output);
    Buffer += TEXT(" \\\\\n");
    WriteInputOutputPinPairs(Node, InputPinIndex + 1, OutputPinIndex + 1);
}

void UConcordModelGraphTikzExporter::WritePin(const UEdGraphPin* Pin)
{
    Buffer += TEXT("\\node [pin] (");
    WritePinID(Pin);
    Buffer += TEXT(") {}; \\node [");
    if (Pin->Direction == EGPD_Input) Buffer += TEXT("inlabel");
    else Buffer += TEXT("outlabel");
    Buffer += TEXT("] {");
    WritePinLabel(Pin);
    Buffer += TEXT("};");
}

void UConcordModelGraphTikzExporter::WritePinPlaceholder(EEdGraphPinDirection Direction)
{
    Buffer += TEXT("\\node [");
    if (Direction == EGPD_Input) Buffer += TEXT("inlabel");
    else Buffer += TEXT("outlabel");
    Buffer += TEXT("] {}; ");
}

void UConcordModelGraphTikzExporter::WritePinLabel(const UEdGraphPin* Pin)
{
    if (Pin->Direction == EGPD_Input || !Pin->GetOwningNode()->IsA<UConcordModelGraphTransformer>())
    {
        if (Pin->PinType.PinCategory == "RV") Buffer += FString::Printf(TEXT("RV %d"), Pin->PinName.GetNumber());
        else Buffer += Pin->GetOwningNode()->GetPinDisplayName(Pin).ToString();
        if (Pin->PinName == "All") Buffer += TEXT(" RVs");
    }
}

void UConcordModelGraphTikzExporter::WriteDetails(const UEdGraphNode* Node)
{
    WriteDetail(TEXT("Shape"), ConcordShape::ToString(Cast<UConcordModelGraphNode>(Node)->Vertex->GetShape()));
    const UConcordVertex* Vertex = Cast<UConcordModelGraphNode>(Node)->Vertex;
    for (TFieldIterator<FProperty> It(Vertex->GetClass(), EFieldIterationFlags::None); It; ++It)
    {
        if (FFloatProperty* FloatProp = CastField<FFloatProperty>(*It))
        {
            const float Value = *FloatProp->ContainerPtrToValuePtr<float>(Vertex);
            WriteDetail(FloatProp->GetNameCPP(), FString::Printf(TEXT("%.3f"), Value));
        }
        else if (FIntProperty* IntProp = CastField<FIntProperty>(*It))
        {
            const int32 Value = *IntProp->ContainerPtrToValuePtr<int32>(Vertex);
            WriteDetail(IntProp->GetNameCPP(), FString::Printf(TEXT("%d"), Value));
        }
    }
}

void UConcordModelGraphTikzExporter::WriteDetail(const FString& Name, const FString& Value)
{
    Buffer += TEXT("        \\node [nodedetails] {");
    Buffer += Name;
    Buffer += TEXT(": ");
    Buffer += Value;
    Buffer += TEXT("}; \\\\\n");
}

bool UConcordModelGraphTikzExporter::ShouldWritePin(const UEdGraphPin* Pin) const
{
    if (Pin->PinType.PinCategory == "RV" && !Cast<UConcordModelGraphBox>(Pin->GetOwningNode())->bEnableIndividualPins) return false;
    if (!bHideAdvancedPins) return true;
    if (Pin->PinName == "State Set") return false;
    if (Pin->PinName == "State Set") return false;
    if (Pin->Direction == EGPD_Input && (Pin->GetOwningNode()->IsA<UConcordModelGraphBox>() || Pin->GetOwningNode()->IsA<UConcordModelGraphParameter>())) return false;
    return true;
}
