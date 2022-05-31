// Copyright Jan Klimaschewski. All Rights Reserved.

#include "ConcordInstance.h"
#include "ConcordNativeModel.h"
#include "ConcordModel.h"
#include "ConcordParameter.h"
#include "ConcordOutput.h"
#include "ConcordShape.h"

void UConcordInstance::UpdateCachedInputOutputInfo() const
{
    CachedInputInfo.Reset();
    CachedOutputInfo.Reset();
    if (!Model) return;

    if (const UConcordNativeModel* NativeModel = Cast<UConcordNativeModel>(Model))
    {
        for (const FName& ParameterName : Model->OrderedParameterNames)
        {
            const auto& [ParameterType, BlockPtr] = NativeModel->GetParameterBlock(ParameterName);
            if (!BlockPtr->bLocal)
                CachedInputInfo.Add({ FConcordShape { BlockPtr->Size }, ParameterType, FText::FromName(ParameterName) });
        }

        for (const FName& OutputName : Model->OrderedOutputNames)
        {
            const auto* OutputInfo = NativeModel->GetOutputInfo(OutputName);
            check(OutputInfo);
            CachedOutputInfo.Add(OutputName, MakeTuple(OutputInfo->Num, OutputInfo->Type));
        }
    }
    else if (const UConcordModel* ConcordModel = Cast<UConcordModel>(Model))
    {
        for (const FName& ParameterName : Model->OrderedParameterNames)
        {
            const UConcordParameter* Parameter = ConcordModel->Parameters[ParameterName];
            if (!Parameter->bLocal)
                CachedInputInfo.Add({ FConcordShape { ConcordShape::GetFlatNum(Parameter->GetShape()) }, Parameter->GetType(), FText::FromName(ParameterName) });
        }

        TSet<UConcordVertex*> VisitedVertices;
        for (const auto& NameOutputPair : ConcordModel->Outputs)
        {
            NameOutputPair.Value->SetupGraph(VisitedVertices);
            CachedOutputInfo.Add(NameOutputPair.Key, MakeTuple(ConcordShape::GetFlatNum(NameOutputPair.Value->GetShape()), NameOutputPair.Value->GetType()));
        }
    }
    else checkNoEntry();
}

FText UConcordInstance::GetDisplayName() const
{
    if (!Model) return INVTEXT("(Empty Instance)");
    return FText::FromString(FString::Printf(TEXT("(%s)"), *Model->GetName()));
}

FText UConcordInstance::GetTooltip() const
{
    return INVTEXT("The instanced model is ran and observed before sampling from the instancing model.");
}

TArray<UConcordVertex::FInputInfo> UConcordInstance::GetInputInfo() const
{
    return CachedInputInfo;
}

EConcordValueType UConcordInstance::GetOutputType(const FName& OutputName) const
{
    const auto* OutputInfo = CachedOutputInfo.Find(OutputName);
    if (!OutputInfo) return EConcordValueType::Error;
    return OutputInfo->Get<EConcordValueType>();
}

UConcordInstanceOutput* UConcordInstance::GetOutputTransformer(const FName& OutputName)
{
    UConcordInstanceOutput* OutputTransformer = FindObjectFast<UConcordInstanceOutput>(this, OutputName, true);
    if (!OutputTransformer) OutputTransformer = NewObject<UConcordInstanceOutput>(this, OutputName);
    return OutputTransformer;
}

UConcordVertex::FSetupResult UConcordInstance::Setup(TOptional<FString>& OutErrorMessage)
{
    // Instance vertex itself has no shape/type, see UConcordInstanceOutput
    return {};
}

FConcordSharedExpression UConcordInstanceOutput::GetExpression(const FConcordMultiIndex& MultiIndex) const
{
    return ExpressionDelegate.Execute(MultiIndex);
}

UConcordVertex::FSetupResult UConcordInstanceOutput::Setup(TOptional<FString>& OutErrorMessage)
{
    // Note that this setup is only called in editor for type/shape preview, the compiler does manual setup
    if (!GetInstance()->Model) { OutErrorMessage = TEXT("Instance has no model set."); return {}; };
    const auto* OutputInfo = GetInstance()->CachedOutputInfo.Find(GetFName());
    if (!OutputInfo) { OutErrorMessage = TEXT("Output is missing."); return {}; };
    return { { OutputInfo->Get<int32>() }, OutputInfo->Get<EConcordValueType>() };
}
