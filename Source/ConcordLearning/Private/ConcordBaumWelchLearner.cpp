// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "ConcordBaumWelchLearner.h"
#include <random>

FConcordBaumWelchLearner::FConcordBaumWelchLearner(const TSharedRef<TArray<FConcordCrateData>>& InDataset, FConcordBaumWelchLearnerSettings InSettings)
    : FConcordLearner(InDataset)
    , Settings(MoveTemp(InSettings))
    , SumProduct(GetFactorGraph(), GetExpressionContext())
    , HiddenBlock(GetFactorGraph()->GetVariationBlocks()[Settings.Names.HiddenBox])
    , HiddenStateCount(GetFactorGraph()->GetStateCount(HiddenBlock.Offset))
{
    Mutate(Settings.InitDeviation);
    SumProduct.Init();
    Alpha.SetNumUninitialized(HiddenStateCount);
    AlphaMarg.SetNumUninitialized(HiddenStateCount);
    Beta.SetNumUninitialized(HiddenStateCount);
    EmissionNumerators.SetNum(Settings.Names.Emissions.Num());
}

FConcordCrateData FConcordBaumWelchLearner::GetCrate() const
{
    FConcordCrateData ParameterCrate;
    for (const FName& BlockName : GetFactorGraph()->GetTrainableFloatParameterBlockNames())
    {
        const FConcordFactorGraphBlock& Block = GetFactorGraph()->GetParameterBlocks<float>()[BlockName];
        FConcordFloatBlock& FloatBlock = ParameterCrate.FloatBlocks.Add(BlockName);
        FloatBlock.Values.Reserve(Block.Size);
        for (int32 Index = Block.Offset; Index < Block.Offset + Block.Size; ++Index)
            FloatBlock.Values.Add(GetEnvironment()->GetStagingFloatParameters()[Index]);
    }
    return MoveTemp(ParameterCrate);
}

void FConcordBaumWelchLearner::OnConvergence()
{
    Mutate(Settings.MutationDeviation);
}

void FConcordBaumWelchLearner::Mutate(double Deviation)
{
    if (Deviation == 0.0) return;
    std::mt19937 Rng{std::random_device()()};
    std::normal_distribution<> Distribution(0.0, Deviation);
    for (const FName& Name : GetFactorGraph()->GetTrainableFloatParameterBlockNames())
    {
        const auto& Block = GetFactorGraph()->GetParameterBlocks<float>()[Name];
        for (int32 Index = Block.Offset; Index < Block.Offset + Block.Size; ++Index)
            GetEnvironment()->GetMutableStagingFloatParameters()[Index] += Distribution(Rng);
    }
}

double FConcordBaumWelchLearner::UpdateAndGetLoss()
{
    SumProduct.RunInward();
    const double LogZ = log(SumProduct.GetZ());

    double LogOs = 0.0;
    InitialNumerators.Reset(); InitialNumerators.AddZeroed(HiddenStateCount);
    TransitionNumerators.Reset(); TransitionNumerators.AddZeroed(HiddenStateCount * HiddenStateCount);
    for (int32 EmissionIndex = 0; EmissionIndex < Settings.Names.Emissions.Num(); ++EmissionIndex)
    {
        const auto& ObservedBlock = GetFactorGraph()->GetVariationBlocks()[Settings.Names.Emissions[EmissionIndex].ObservedBox];
        const int32 ObservedStateCount = GetFactorGraph()->GetStateCount(ObservedBlock.Offset);
        EmissionNumerators[EmissionIndex].Reset(); EmissionNumerators[EmissionIndex].AddZeroed(HiddenStateCount * ObservedStateCount);
    }
    Denominators.Reset(); Denominators.AddZeroed(HiddenStateCount);
    EmissionDenominatorSummands.Reset(); EmissionDenominatorSummands.AddZeroed(HiddenStateCount);
    for (const FConcordCrateData& Crate : Dataset.Get())
    {
        GetEnvironment()->SetCrate(Crate);
        SumProduct.RunInward();
        LogOs += log(SumProduct.GetZ());
        SumProduct.RunOutward();
        for (int32 LocalRandomVariableIndex = 0; LocalRandomVariableIndex < HiddenBlock.Size - 1; ++LocalRandomVariableIndex)
        {
            const int32 HiddenIndex = HiddenBlock.Offset + LocalRandomVariableIndex;
            auto NextHandle = GetNextHandle(HiddenIndex);
            SetAlpha(HiddenIndex, NextHandle);
            for (int32 AlphaValue = 0; AlphaValue < HiddenStateCount; ++AlphaValue)
            {
                for (int32 EmissionIndex = 0; EmissionIndex < Settings.Names.Emissions.Num(); ++EmissionIndex)
                {
                    const auto& Emission = Settings.Names.Emissions[EmissionIndex];
                    const auto& ObservedBlock = GetFactorGraph()->GetVariationBlocks()[Emission.ObservedBox];
                    const int32 ObservedStateCount = GetFactorGraph()->GetStateCount(ObservedBlock.Offset);
                    const int32 ObservedIndex = ObservedBlock.Offset + Emission.Offset + LocalRandomVariableIndex * Emission.Stride;
                    EmissionNumerators[EmissionIndex][AlphaValue * ObservedStateCount + GetVariation()[ObservedIndex]] += AlphaMarg[AlphaValue];
                }
                Denominators[AlphaValue] += AlphaMarg[AlphaValue];
                SetBeta(HiddenIndex + 1, AlphaValue, NextHandle, GetEnvironment()->GetStagingFloatParametersView(GetFactorGraph()->GetParameterBlocks<float>()[Settings.Names.Transition]));
                for (int32 BetaValue = 0; BetaValue < HiddenStateCount; ++BetaValue)
                    TransitionNumerators[AlphaValue * HiddenStateCount + BetaValue] += AlphaMarg[AlphaValue] * Beta[BetaValue];
            }

            if (LocalRandomVariableIndex == 0)
                for (int32 Value = 0; Value < HiddenStateCount; ++Value)
                    InitialNumerators[Value] += AlphaMarg[Value];
        }
        // explicit final iteration for emission probs
        const int32 HiddenIndex = HiddenBlock.Offset + HiddenBlock.Size - 1;
        SetAlpha(HiddenIndex, nullptr);
        for (int32 AlphaValue = 0; AlphaValue < HiddenStateCount; ++AlphaValue)
        {
            for (int32 EmissionIndex = 0; EmissionIndex < Settings.Names.Emissions.Num(); ++EmissionIndex)
            {
                const auto& Emission = Settings.Names.Emissions[EmissionIndex];
                const auto& ObservedBlock = GetFactorGraph()->GetVariationBlocks()[Emission.ObservedBox];
                const int32 ObservedStateCount = GetFactorGraph()->GetStateCount(ObservedBlock.Offset);
                const int32 ObservedIndex = ObservedBlock.Offset + Emission.Offset + (HiddenBlock.Size - 1) * Emission.Stride;
                EmissionNumerators[EmissionIndex][AlphaValue * ObservedStateCount + GetVariation()[ObservedIndex]] += AlphaMarg[AlphaValue];
            }
            EmissionDenominatorSummands[AlphaValue] += AlphaMarg[AlphaValue];
        }
        GetEnvironment()->UnsetCrate(Crate);
    }

    const auto& InitialBlock = GetFactorGraph()->GetParameterBlocks<float>()[Settings.Names.Initial];
    for (int32 AlphaValue = 0; AlphaValue < HiddenStateCount; ++AlphaValue)
        GetEnvironment()->GetMutableStagingFloatParameters()[InitialBlock.Offset + AlphaValue] = ProbToScore(InitialNumerators[AlphaValue] / Dataset->Num());

    const auto& TransitionBlock = GetFactorGraph()->GetParameterBlocks<float>()[Settings.Names.Transition];
    for (int32 AlphaValue = 0; AlphaValue < HiddenStateCount; ++AlphaValue)
        for (int32 BetaValue = 0; BetaValue < HiddenStateCount; ++BetaValue)
        {
            const int32 TransitionIndex = AlphaValue * HiddenStateCount + BetaValue;
            GetEnvironment()->GetMutableStagingFloatParameters()[TransitionBlock.Offset + TransitionIndex] = ProbToScore(TransitionNumerators[TransitionIndex] / Denominators[AlphaValue]);
        }

    for (int32 AlphaValue = 0; AlphaValue < HiddenStateCount; ++AlphaValue)
        Denominators[AlphaValue] += EmissionDenominatorSummands[AlphaValue];

    for (int32 EmissionIndex = 0; EmissionIndex < Settings.Names.Emissions.Num(); ++EmissionIndex)
    {
        const auto& Emission = Settings.Names.Emissions[EmissionIndex];
        const auto& EmissionBlock = GetFactorGraph()->GetParameterBlocks<float>()[Emission.Emission];
        const auto& ObservedBlock = GetFactorGraph()->GetVariationBlocks()[Emission.ObservedBox];
        const int32 ObservedStateCount = GetFactorGraph()->GetStateCount(ObservedBlock.Offset);
        for (int32 AlphaValue = 0; AlphaValue < HiddenStateCount; ++AlphaValue)
            for (int32 ObservedValue = 0; ObservedValue < ObservedStateCount; ++ObservedValue)
            {
                const int32 EmissionParameterIndex = AlphaValue * ObservedStateCount + ObservedValue;
                const double Prob = EmissionNumerators[EmissionIndex][EmissionParameterIndex] / Denominators[AlphaValue];
                GetEnvironment()->GetMutableStagingFloatParameters()[EmissionBlock.Offset + EmissionParameterIndex] = ProbToScore(Prob);
            }
    }

    return -(LogOs / Dataset->Num() - LogZ);
}

FConcordExpressionContextMutable<float> FConcordBaumWelchLearner::GetExpressionContext() const
{
    return { GetEnvironment()->GetMutableStagingVariation(), GetEnvironment()->GetStagingMask(), GetEnvironment()->GetStagingIntParameters(), GetEnvironment()->GetStagingFloatParameters() };
}

const FConcordFactorHandleBase<float>* FConcordBaumWelchLearner::GetNextHandle(int32 FlatRandomVariableIndex) const
{
    const FConcordFactorHandleBase<float>* NextHandle = nullptr;
    for (const auto* NeighboringHandle : GetFactorGraph()->GetNeighboringHandles(FlatRandomVariableIndex))
        if (NeighboringHandle->GetNeighboringFlatRandomVariableIndices().Contains(FlatRandomVariableIndex + 1))
        {
            NextHandle = NeighboringHandle;
            break;
        }
    return NextHandle;
}

void FConcordBaumWelchLearner::SetAlpha(int32 FlatRandomVariableIndex, const FConcordFactorHandleBase<float>* NextHandle)
{
    double Sum = 0.0;
    for (int32 Value = 0; Value < HiddenStateCount; ++Value)
    {
        Alpha[Value] = 1;
        AlphaMarg[Value] = 1;
        for (const auto* NeighboringHandle : GetFactorGraph()->GetNeighboringHandles(FlatRandomVariableIndex))
        {
            const double Factor = SumProduct.GetVariableMessageFactors()[FlatRandomVariableIndex][NeighboringHandle][Value];
            if (NeighboringHandle != NextHandle) Alpha[Value] *= Factor;
            AlphaMarg[Value] *= Factor;
            Sum += AlphaMarg[Value];
        }
    }
    for (double& Prob : AlphaMarg) Prob /= Sum;
}

void FConcordBaumWelchLearner::SetBeta(int32 FlatRandomVariableIndex, int32 AlphaValue, const FConcordFactorHandleBase<float>* PreviousHandle, const TArrayView<float>& TransitionScores)
{
    double Sum = 0.0;
    for (int32 Value = 0; Value < Beta.Num(); ++Value)
    {
        Beta[Value] = Alpha[AlphaValue];
        Beta[Value] *= exp(double(TransitionScores[AlphaValue * Beta.Num() + Value]));
        for (const auto* NeighboringHandle : GetFactorGraph()->GetNeighboringHandles(FlatRandomVariableIndex))
            if (NeighboringHandle != PreviousHandle)
                Beta[Value] *= SumProduct.GetVariableMessageFactors()[FlatRandomVariableIndex][NeighboringHandle][Value];
        Sum += Beta[Value];
    }
    for (double& Prob : Beta) Prob /= Sum;
}

float FConcordBaumWelchLearner::ProbToScore(double Prob)
{
    return 5.0f + log(FMath::Max(1e-8, Prob));
}
