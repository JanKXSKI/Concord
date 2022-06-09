// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "ConcordPatternCustomization.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SButton.h"
#include "Editor.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "IAudioParameterInterfaceRegistry.h"
#include "ConcordNativeModel.h"

void FConcordPatternCustomization::CustomizeDetails(IDetailLayoutBuilder& LayoutBuilder)
{
	TArray<TWeakObjectPtr<UObject>> CustomizedObjects;
	LayoutBuilder.GetObjectsBeingCustomized(CustomizedObjects);
	if (!CustomizedObjects.IsEmpty())
	{
		if (auto ModelBase = Cast<UConcordModelBase>(CustomizedObjects[0]))
			CustomizedPattern = ModelBase->LatestPatternSampledFromEditor;
		else
			CustomizedPattern = Cast<UConcordPattern>(CustomizedObjects[0]);
	}

	LayoutBuilder.EditCategory("Preview").AddCustomRow(INVTEXT("Play")).WholeRowContent()
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SButton)
			.VAlign(VAlign_Center)
			.Text(INVTEXT("Play"))
			.OnClicked_Lambda([&]{ Play(); return FReply::Handled(); })
		]
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SButton)
			.VAlign(VAlign_Center)
			.Text(INVTEXT("Stop"))
			.OnClicked_Lambda([&]{ Stop(); return FReply::Handled(); })
		]
	];
}

void FConcordPatternCustomization::Play() const
{
	if (!CustomizedPattern.IsValid()) return;
	if (!CustomizedPattern->PreviewSound) return;
	if (!CustomizedPattern->PreviewSound->ImplementsParameterInterface(MakeShared<Audio::FParameterInterface>("Concord", Audio::FParameterInterface::FVersion { 1, 0 }, *USoundBase::StaticClass()))) return;
	UAudioComponent* PreviewAudioComponent = GEditor->PlayPreviewSound(CustomizedPattern->PreviewSound);
	PreviewAudioComponent->SetObjectParameter("Concord.Pattern", CustomizedPattern.Get());
	PreviewAudioComponent->SetIntParameter("Concord.StartLine", CustomizedPattern->PreviewStartLine);
	PreviewAudioComponent->SetIntParameter("Concord.BPM", CustomizedPattern->PreviewBPM);
	PreviewAudioComponent->SetIntParameter("Concord.LinesPerBeat", CustomizedPattern->PreviewLinesPerBeat);
	PreviewAudioComponent->SetIntParameter("Concord.NumberOfLines", CustomizedPattern->PreviewNumberOfLines);
	PreviewAudioComponent->SetTriggerParameter("Concord.Start");
	CustomizedPattern->StartPreview();
}

void FConcordPatternCustomization::Stop() const
{
	GEditor->ResetPreviewAudioComponent();
	CustomizedPattern->StopPreview();
}
