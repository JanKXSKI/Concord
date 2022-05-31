// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

class FConcordModelEditorCommands : public TCommands<FConcordModelEditorCommands>
{
public:
    FConcordModelEditorCommands()
        : TCommands("ConcordModelEditor", NSLOCTEXT("Contexts", "ConcordModelEditor", "Concord Model Editor"), NAME_None, "ConcordEditorStyle")
    {}

    TSharedPtr<FUICommandInfo> SampleVariation;
    TSharedPtr<FUICommandInfo> SelectedOutputsToCrate;

    void RegisterCommands() override
    {
#define LOCTEXT_NAMESPACE "ConcordModelEditorCommands"
        UI_COMMAND(SampleVariation, "Sample Variation", "Samples a new variation to display in the model editor.", EUserInterfaceActionType::Button, FInputChord(EKeys::SpaceBar));
        UI_COMMAND(SelectedOutputsToCrate, "Selected Outputs to Crate", "Writes the values that have been most recently computed for the selected outputs to a crate asset.", EUserInterfaceActionType::Button, FInputChord());
#undef LOCTEXT_NAMESPACE
    }
};
