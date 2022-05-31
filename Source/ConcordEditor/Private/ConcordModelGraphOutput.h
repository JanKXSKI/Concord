// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordModelGraphConsumer.h"
#include "ConcordOutput.h"
#include "SConcordModelGraphSink.h"
#include "ConcordModelGraphOutput.generated.h"

UCLASS()
class UConcordModelGraphOutput : public UConcordModelGraphConsumer
{
    GENERATED_BODY()
public:
    UConcordModelGraphOutput() : bDisplayOutput(false) { bCanRenameNode = true; bDisplayOutput = true; }
    void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
    void OnRenameNode(const FString& NewNameString) override;
    void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    void AllocateDefaultPins() override;
    TSharedPtr<SGraphNode> CreateVisualWidget() override;
    void PostEditImport() override;
    void DestroyNode() override;

    UPROPERTY(EditAnywhere, Category = "Output")
    bool bDisplayOutput;

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnIntOutputValuesChanged, const TArray<int32>&);
    FOnIntOutputValuesChanged OnIntOutputValuesChanged;
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnFloatOutputValuesChanged, const TArray<float>&);
    FOnFloatOutputValuesChanged OnFloatOutputValuesChanged;
    TArray<FConcordValue> CurrentOutputValues;

    UConcordOutput* GetOutput() { return Cast<UConcordOutput>(Vertex); }
};

class SConcordModelGraphOutput : public SConcordModelGraphSink
{
private:
    void AddSinkDelegates() override;
    TSharedRef<SWidget> CreateAdditionalSinkContent() override;
    void OnIntOutputValuesChanged(const TArray<int32>& Values);
    void OnFloatOutputValuesChanged(const TArray<float>& Values);
    EVisibility GetOutputValuesTextVisibility() const;

    TSharedPtr<STextBlock> OutputValuesText;
};

USTRUCT()
struct FConcordModelGraphAddNodeAction_NewOutput : public FConcordModelGraphAddNodeAction
{
    GENERATED_BODY()

    FConcordModelGraphAddNodeAction_NewOutput();
    UEdGraphNode* MakeNode(UEdGraph* ParentGraph, UEdGraphPin* FromPin) override;
};
