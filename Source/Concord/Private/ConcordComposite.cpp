// Copyright Jan Klimaschewski. All Rights Reserved.

#include "ConcordComposite.h"
#include "ConcordModel.h"
#include "ConcordParameter.h"
#include "ConcordOutput.h"
#include "Transformers/ConcordTransformerValue.h"

FText UConcordComposite::GetDisplayName() const
{
    if (!Model) return INVTEXT("(Empty Composite)");
    return FText::FromString(FString::Printf(TEXT("(%s)"), *Model->GetName()));
}

FText UConcordComposite::GetTooltip() const
{
    return INVTEXT("Composites allow for the composition of Concord models.");
}

TArray<UConcordVertex::FInputInfo> UConcordComposite::GetInputInfo() const
{
    if (!Model) return {};
    TArray<FInputInfo> InputInfo;
    for (const FName& ParameterName : Model->OrderedParameterNames)
    {
        const UConcordParameter* Parameter = Model->Parameters[ParameterName];
        if (!Parameter->bLocal)
            InputInfo.Add({ {}, Parameter->GetType(), FText::FromName(ParameterName) });
    }
    return MoveTemp(InputInfo);
}

EConcordValueType UConcordComposite::GetOutputType(const FName& OutputName) const
{
    if (Model)
        if (UConcordOutput* const* Output = Model->Outputs.Find(OutputName))
            return (*Output)->GetType();
    return EConcordValueType::Error;
}

UConcordCompositeOutput* UConcordComposite::GetOutputTransformer(const FName& OutputName)
{
    UConcordCompositeOutput* OutputTransformer = FindObjectFast<UConcordCompositeOutput>(this, OutputName, true);
    if (!OutputTransformer) OutputTransformer = NewObject<UConcordCompositeOutput>(this, OutputName);
    return OutputTransformer;
}

UConcordVertex::FSetupResult UConcordComposite::Setup(TOptional<FString>& OutErrorMessage)
{
    // Composite vertex itself has no shape/type, see UConcordCompositeOutput
    return {};
}

FConcordSharedExpression UConcordCompositeOutput::GetExpression(const FConcordMultiIndex& MultiIndex) const
{
    return ExpressionDelegate.Execute(MultiIndex);
}

UConcordVertex::FSetupResult UConcordCompositeOutput::Setup(TOptional<FString>& OutErrorMessage)
{
    // Note that this setup is only called in editor for type/shape preview, the compiler does manual setup
    if (!GetComposite()->Model) { OutErrorMessage = TEXT("Composite has no model set."); return {}; };
    UConcordOutput* const* Output = GetComposite()->Model->Outputs.Find(GetFName());
    if (!Output) { OutErrorMessage = TEXT("Missing output."); return {}; };
    return { (*Output)->GetShape(), (*Output)->GetType() };
}
