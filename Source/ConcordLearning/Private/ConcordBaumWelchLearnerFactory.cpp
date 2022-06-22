// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "ConcordBaumWelchLearnerFactory.h"
#include "Misc/MessageDialog.h"

UConcordBaumWelchLearnerFactory::UConcordBaumWelchLearnerFactory()
    : bSetDefaultCrates(false)
    , ParameterInitDeviation(1.0)
    , ParameterMutationDeviation(1.0)
    , LearningRate(1.0)
    , StabilityBias(2.0f)
{}

bool UConcordBaumWelchLearnerFactory::CheckAndInit()
{
    TOptional<FConcordError> Error;
    FactorGraph = GetModel()->GetFactorGraph(EConcordCycleMode::Error, Error);
    if (Error)
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::Format(INVTEXT("Error getting factor graph: %s."), FText::FromString(Error.GetValue().Message)));
        return false;
    }

    if (!SetNames())
    {
        FMessageDialog::Open(EAppMsgType::Ok, INVTEXT("The Baum Welch learner can only train models that have a single box of hidden variables with one or more emisisons."));
        return false;
    }

    if (!CheckDataset())
    {
        FMessageDialog::Open(EAppMsgType::Ok, INVTEXT("Not all crates in the dataset observe all values that were designated observations in the model."));
        return false;
    }

    return true;
}

TSharedPtr<FConcordLearner> UConcordBaumWelchLearnerFactory::CreateLearner()
{
    return MakeShared<FConcordBaumWelchLearner>(Dataset.ToSharedRef(), MakeSettings());
}

TSharedRef<const FConcordFactorGraph<float>> UConcordBaumWelchLearnerFactory::GetFactorGraph() const
{
    return FactorGraph.ToSharedRef();
}

TUniquePtr<FConcordFactorGraphEnvironment<float>> UConcordBaumWelchLearnerFactory::MakeEnvironment() const
{
    auto Environment = MakeUnique<FConcordFactorGraphEnvironment<float>>(GetFactorGraph());
    if (bSetDefaultCrates) for (const UConcordCrate* Crate : GetModel()->DefaultCrates) if (Crate) Environment->SetCrate(Crate->CrateData);
    return MoveTemp(Environment);
}

FConcordBaumWelchLearnerSettings UConcordBaumWelchLearnerFactory::MakeSettings() const
{
    FConcordBaumWelchLearnerSettings Settings { GetFactorGraph(), MakeEnvironment(), Names };
    Settings.InitDeviation = ParameterInitDeviation;
    Settings.MutationDeviation = ParameterMutationDeviation;
    Settings.LearningRate = LearningRate;
    Settings.StabilityBias = StabilityBias;
    return MoveTemp(Settings);
}

bool UConcordBaumWelchLearnerFactory::SetNames()
{
    TOptional<FConcordBaumWelchNames> OptionalNames;
    for (FName BlockName : FactorGraph->GetTrainableFloatParameterBlockNames())
    {
        const FString BlockString = BlockName.ToString();
        if (BlockString.EndsWith(TEXT(".Initial")))
        {
            const FName HiddenBoxName = FName(BlockString.LeftChop(8));
            if (!FactorGraph->GetVariationBlocks().Contains(HiddenBoxName)) return false;
            if (OptionalNames && OptionalNames.GetValue().HiddenBox != HiddenBoxName) return false;
            else if (!OptionalNames) OptionalNames = FConcordBaumWelchNames { HiddenBoxName };
            OptionalNames.GetValue().Initial = BlockName;
        }
        else if (BlockString.EndsWith(TEXT(".Transition")))
        {
            const FName HiddenBoxName = FName(BlockString.LeftChop(11));
            if (!FactorGraph->GetVariationBlocks().Contains(HiddenBoxName)) return false;
            if (OptionalNames && OptionalNames.GetValue().HiddenBox != HiddenBoxName) return false;
            else if (!OptionalNames) OptionalNames = FConcordBaumWelchNames { HiddenBoxName };
            OptionalNames.GetValue().Transition = BlockName;
        }
        else if (BlockString.EndsWith(TEXT(".Emission")))
        {
            FString BoxesString = BlockString.LeftChop(9);
            int32 DotIndex;
            if (!BoxesString.FindLastChar(TEXT('.'), DotIndex)) return false;
            int32 Stride = -1;
            LexFromString(Stride, *BoxesString.RightChop(DotIndex + 1));
            if (Stride < 0) return false;
            BoxesString.LeftInline(DotIndex, false);
            if (!BoxesString.FindLastChar(TEXT('.'), DotIndex)) return false;
            int32 Offset = -1;
            LexFromString(Offset, *BoxesString.RightChop(DotIndex + 1));
            if (Offset < 0) return false;
            BoxesString.LeftInline(DotIndex, false);
            FString HiddenBoxString = BoxesString;
            if (!HiddenBoxString.FindLastChar(TEXT('.'), DotIndex)) return false;
            HiddenBoxString.LeftInline(DotIndex, false);
            const FName HiddenBoxName = FName(HiddenBoxString);
            const int32 HiddenBoxNameLength = HiddenBoxString.Len();
            if (!HiddenBoxString.FindLastChar(TEXT('.'), DotIndex)) DotIndex = -1;
            HiddenBoxString.LeftInline(DotIndex + 1, false);
            const FName ObservedBoxName = FName(HiddenBoxString + BoxesString.RightChop(HiddenBoxNameLength + 1));
            if (!FactorGraph->GetVariationBlocks().Contains(HiddenBoxName)) return false;
            if (!FactorGraph->GetVariationBlocks().Contains(ObservedBoxName)) return false;
            if (OptionalNames && OptionalNames.GetValue().HiddenBox != HiddenBoxName) return false;
            else if (!OptionalNames) OptionalNames = FConcordBaumWelchNames { HiddenBoxName };
            OptionalNames.GetValue().Emissions.Add({ ObservedBoxName, BlockName, Offset, Stride });
        }
        else return false;
    }

    if (!OptionalNames ||
        OptionalNames.GetValue().HiddenBox.IsNone() ||
        OptionalNames.GetValue().Initial.IsNone() ||
        OptionalNames.GetValue().Transition.IsNone() ||
        OptionalNames.GetValue().Emissions.IsEmpty())
        return false;

    Names = OptionalNames.GetValue();

    return true;
}

bool UConcordBaumWelchLearnerFactory::CheckDataset() const
{
    for (const FConcordCrateData& Crate : *Dataset.Get())
        for (const auto& EmissionNames : Names.Emissions)
        {
            if (!Crate.IntBlocks.Contains(EmissionNames.ObservedBox)) return false;
            if (Crate.IntBlocks[EmissionNames.ObservedBox].Values.Num() < FactorGraph->GetVariationBlocks()[EmissionNames.ObservedBox].Size) return false;
        }
    return true;
}
