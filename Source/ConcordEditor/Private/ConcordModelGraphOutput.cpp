// Copyright Jan Klimaschewski. All Rights Reserved.

#include "ConcordModelGraphOutput.h"
#include "ConcordModel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"

void UConcordModelGraphOutput::GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
    if (ContextMenuBuilder.FromPin &&
        ContextMenuBuilder.FromPin->Direction != EGPD_Output) return;
    ContextMenuBuilder.AddAction(MakeShared<FConcordModelGraphAddNodeAction_NewOutput>());
}

void UConcordModelGraphOutput::OnRenameNode(const FString& NewNameString)
{
    UConcordModelGraphConsumer::OnRenameNode(NewNameString);
    OnInputOutputInterfaceChanged();
}

void UConcordModelGraphOutput::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    UConcordModelGraphConsumer::PostEditChangeProperty(PropertyChangedEvent);
    if (PropertyChangedEvent.GetPropertyName() != "bDisplayOutput")
        OnInputOutputInterfaceChanged();
}

void UConcordModelGraphOutput::AllocateDefaultPins()
{
    UConcordModelGraphConsumer::AllocateDefaultPins();
    OnInputOutputInterfaceChanged();
}

TSharedPtr<SGraphNode> UConcordModelGraphOutput::CreateVisualWidget()
{
    return SNew(SConcordModelGraphOutput, this)
    .WithPinDefaultValues(true)
    .WithPinLabels(true);
}

void UConcordModelGraphOutput::PostEditImport()
{
    UConcordModelGraphConsumer::PostEditImport();
    OnInputOutputInterfaceChanged();
}

void UConcordModelGraphOutput::DestroyNode()
{
    GetModel()->Modify();
    GetModel()->Outputs.Remove(Name);
    GetModel()->OrderedOutputNames.RemoveSingle(Name);
    GetModel()->Outputs.Shrink();
    Super::DestroyNode();
    OnInputOutputInterfaceChanged();
}

void SConcordModelGraphOutput::AddSinkDelegates()
{
    UConcordModelGraphOutput* OutputNode = Cast<UConcordModelGraphOutput>(GraphNode);
    OutputNode->OnNodeChanged.AddSP(this, &SConcordModelGraphOutput::UpdateGraphNode);
    OutputNode->OnIntOutputValuesChanged.AddSP(this, &SConcordModelGraphOutput::OnIntOutputValuesChanged);
    OutputNode->OnFloatOutputValuesChanged.AddSP(this, &SConcordModelGraphOutput::OnFloatOutputValuesChanged);
}

TSharedRef<SWidget> SConcordModelGraphOutput::CreateAdditionalSinkContent()
{
    FTextBlockStyle TextBlockStyle = FTextBlockStyle::GetDefault();
    TextBlockStyle.SetFontSize(10);
    TextBlockStyle.SetTypefaceFontName("Bold");
    TextBlockStyle.SetColorAndOpacity(FLinearColor::White);
    SAssignNew(OutputValuesText, STextBlock)
    .TextStyle(&TextBlockStyle)
    .Justification(ETextJustify::Center);
    return SNew(SBox)
    .Padding(FMargin(0.0f, 8.0f, 8.0f, 4.0f))
    .Visibility(this, &SConcordModelGraphOutput::GetOutputValuesTextVisibility)
    [ OutputValuesText.ToSharedRef() ];
}

void SConcordModelGraphOutput::OnIntOutputValuesChanged(const TArray<int32>& Values)
{
    if (Values.IsEmpty()) { OutputValuesText->SetText(FText()); return; }
    FString ValuesText { "", Values.Num() * 2 };
    for (int32 ValueIndex = 0; ValueIndex < Values.Num() - 1; ++ValueIndex)
        ValuesText += LexToString(Values[ValueIndex]) + "\n";
    ValuesText += LexToString(Values.Last());
    OutputValuesText->SetText(FText::FromString(ValuesText));
}

void SConcordModelGraphOutput::OnFloatOutputValuesChanged(const TArray<float>& Values)
{
    if (Values.IsEmpty()) { OutputValuesText->SetText(FText()); return; }
    FString ValuesText { "", Values.Num() * 2 };
    for (int32 ValueIndex = 0; ValueIndex < Values.Num() - 1; ++ValueIndex)
        ValuesText += FString::Printf(TEXT("%.2f"), Values[ValueIndex]) + "\n";
    ValuesText += FString::Printf(TEXT("%.2f"), Values.Last());
    OutputValuesText->SetText(FText::FromString(ValuesText));
}

EVisibility SConcordModelGraphOutput::GetOutputValuesTextVisibility() const
{
    if (Cast<UConcordModelGraphOutput>(GraphNode)->bDisplayOutput)
        return EVisibility::Visible;
    return EVisibility::Collapsed;
}

FConcordModelGraphAddNodeAction_NewOutput::FConcordModelGraphAddNodeAction_NewOutput()
    : FConcordModelGraphAddNodeAction(FText(),
                                      INVTEXT("Output"),
                                      Cast<UConcordVertex>(UConcordOutput::StaticClass()->GetDefaultObject())->GetTooltip(),
                                      INVTEXT("Add New Concord Output"))
{}

UEdGraphNode* FConcordModelGraphAddNodeAction_NewOutput::MakeNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin)
{
    FGraphNodeCreator<UConcordModelGraphOutput> NodeCreator(*ParentGraph);
    UConcordModelGraphOutput* NewOutputNode = NodeCreator.CreateUserInvokedNode();
    NewOutputNode->Name = FName("NewOutput");

    NewOutputNode->Vertex = NewObject<UConcordOutput>(NewOutputNode->GetModel(), NAME_None, RF_Transactional);
    NewOutputNode->SyncModelName(false);

    NodeCreator.Finalize();
    return NewOutputNode;
}
