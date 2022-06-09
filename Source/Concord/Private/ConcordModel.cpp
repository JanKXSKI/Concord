// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "ConcordModel.h"
#include "ConcordCompiler.h"
#include "ConcordBox.h"
#include "ConcordComposite.h"
#include "ConcordInstance.h"
#include "ConcordParameter.h"

TSharedPtr<const FConcordFactorGraph<float>> UConcordModel::GetFactorGraph(EConcordCycleMode CycleMode, TOptional<FConcordError>& ErrorOut) const
{
    FConcordCompiler::FResult CompilerResult = FConcordCompiler::Compile(this, CycleMode);
    ErrorOut = CompilerResult.Error;
    if (ErrorOut) return {};
    return MoveTemp(CompilerResult.FactorGraph);
}

bool UConcordModel::IsNameTaken(const FName& Name) const
{
    return Boxes.Contains(Name) || Outputs.Contains(Name) || Parameters.Contains(Name) || Composites.Contains(Name) || Instances.Contains(Name);
}

FName UConcordModel::SanitizeName(const FName& Name)
{
    if (Name.IsNone()) return FName("_");
    FString SanitizedString = Name.ToString().Replace(TEXT("."), TEXT(""));
    if (SanitizedString.IsEmpty()) SanitizedString = TEXT("_");
    return FName(SanitizedString);
}

TArray<UConcordVertex*> UConcordModel::GetUpstreamSources() const
{
    TArray<UConcordVertex*> UpstreamSources;
    UpstreamSources.Reserve(Boxes.Num() + Composites.Num() + Instances.Num() + Parameters.Num());
    for (const TPair<FName, UConcordBox*>& NameBoxPair : Boxes)
        NameBoxPair.Value->GetUpstreamSources(UpstreamSources);
    for (const TPair<FName, UConcordComposite*>& NameCompositePair : Composites)
        NameCompositePair.Value->GetUpstreamSources(UpstreamSources);
    for (const TPair<FName, UConcordInstance*>& NameInstancePair : Instances)
        NameInstancePair.Value->GetUpstreamSources(UpstreamSources);
    for (const TPair<FName, UConcordParameter*>& NameParameterPair : Parameters)
        NameParameterPair.Value->GetUpstreamSources(UpstreamSources);
    return MoveTemp(UpstreamSources);
}
