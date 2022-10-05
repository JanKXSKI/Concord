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
#include "DesktopPlatformModule.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformProcess.h"
#include "MidiFile.h"

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
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SButton)
			.VAlign(VAlign_Center)
			.Text(INVTEXT("Export Midi"))
			.OnClicked_Lambda([&]{ ExportMidi(); return FReply::Handled(); })
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

void FConcordPatternCustomization::ExportMidi() const
{
	if (!CustomizedPattern.IsValid()) return;
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform) return;
	TArray<FString> Filenames;
	DesktopPlatform->SaveFileDialog(FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
									TEXT("Export Midi File"),
									FPlatformProcess::UserDir(),
									TEXT("Pattern.mid"),
									TEXT("MIDI file|*.mid"),
									EFileDialogFlags::None,
									Filenames);
	if (Filenames.IsEmpty()) return;
	UConcordPatternUtilities::ExportMidi(CustomizedPattern.Get(), Filenames[0]);
}

void UConcordPatternUtilities::ExportMidi(UConcordPattern* Pattern, const FString& Filename)
{
	smf::MidiFile File;
	for (const auto& NameTrackPair : Pattern->PatternData.Tracks)
	{
        const int32 TrackIndex = File.addTrack();
		for (const FConcordColumn& Column : NameTrackPair.Value.Columns)
		{
			int32 LastNote = -1;
			for (int32 NoteIndex = 0; NoteIndex < Column.NoteValues.Num(); ++NoteIndex)
			{
				if (Column.NoteValues[NoteIndex] == 0) continue;
				const float Delay = (Column.DelayValues.Num() > NoteIndex) ? FMath::Clamp(Column.DelayValues[NoteIndex], 0, 6) / 6.0f : 0.0f;
				const int32 Tick = (NoteIndex + Delay) / Pattern->PreviewLinesPerBeat * File.getTPQ();
				if (LastNote != -1)
				{
					File.addNoteOff(TrackIndex, Tick, 0, LastNote);
					LastNote = -1;
				}
				if (Column.NoteValues[NoteIndex] > 0)
				{
					const int32 Velocity = (Column.VolumeValues.Num() > NoteIndex) ? FMath::Clamp(Column.VolumeValues[NoteIndex] * 2, 0, 127) : 127;
					File.addNoteOn(TrackIndex, Tick, 0, Column.NoteValues[NoteIndex], Velocity);
					LastNote = Column.NoteValues[NoteIndex];
				}
			}
			if (LastNote != -1)
			{
				const int32 Tick = Column.NoteValues.Num() / Pattern->PreviewLinesPerBeat * File.getTPQ();
				File.addNoteOff(TrackIndex, Tick, 0, LastNote);
			}
		}
	}
	File.sortTracks();
	File.write(TCHAR_TO_UTF8(*Filename));
}
