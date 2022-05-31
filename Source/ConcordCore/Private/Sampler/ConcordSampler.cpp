// Copyright Jan Klimaschewski. All Rights Reserved.

#include "Sampler/ConcordSampler.h"

FConcordSampler::FConcordSampler(TSharedRef<const FConcordFactorGraph<float>> InFactorGraph,
                                 TSharedRef<FConcordFactorGraphEnvironment<float>> InEnvironment,
                                 bool bInMaximizeScore)
    : bMaximizeScore(bInMaximizeScore)
    , FactorGraph(MoveTemp(InFactorGraph))
    , Environment(MoveTemp(InEnvironment))
    , SamplingUtils(&FactorGraph.Get(), GetExpressionContextMutable())
{
    GetVariationFromEnvironment();
}

float FConcordSampler::SampleVariationSync()
{
    checkf(!IsSamplingVariation(), TEXT("Tried to sample a variation synchronously while an asynchronous sampling was in progress"));
    if (IsSamplingVariation()) return 0.0f;
    RunInstanceSamplers();
    return SampleVariation();
}

void FConcordSampler::SampleVariationAsync()
{
    checkf(!IsSamplingVariation(), TEXT("Tried to sample a variation asynchronously while another asynchronous sampling was in progress"));
    if (IsSamplingVariation()) return;
    struct FSampleCallable
    {
        TSharedRef<FConcordSampler> Sampler;
        float operator()() const
        {
            Sampler->RunInstanceSamplers();
            return Sampler->SampleVariation();
        }
    };
    FutureScore = Async(EAsyncExecution::ThreadPool, FSampleCallable { this->AsShared() });
}

bool FConcordSampler::IsSamplingVariation() const
{
    return FutureScore.IsValid();
}

TOptional<float> FConcordSampler::GetScoreIfDoneSampling()
{
    if (!FutureScore.IsReady()) return {};
    const float Score = FutureScore.Get();
    FutureScore.Reset();
    return Score;
}

#if WITH_EDITOR
float FConcordSampler::SampleVariationAndInferMarginalsSync(FConcordProbabilities& OutMarginals)
{
    checkf(!IsSamplingVariation(), TEXT("Tried to sample a variation synchronously while an asynchronous sampling was in progress"));
    if (IsSamplingVariation()) return 0.0f;
    OutMarginals.SetNum(Variation.Num(), false);
    RunInstanceSamplers();
    return SampleVariationAndInferMarginals(OutMarginals);
}
#endif

void FConcordSampler::RunInstanceSamplers()
{
    for (const auto& InstanceNameSamplerPair : FactorGraph->GetInstanceSamplers())
    {
        const TSharedRef<FConcordSampler>& InstanceSampler = InstanceNameSamplerPair.Value;
        for (const auto& NameBlockPair : InstanceSampler->FactorGraph->GetParameterBlocks<int32>())
        {
            const FName OutputName = FName(InstanceNameSamplerPair.Key.ToString() + TEXT(".") + NameBlockPair.Key.ToString() + TEXT(".Source"));
            auto* Output = FactorGraph->GetOutputs().Find(OutputName);
            if (!Output) continue;
            (*Output)->Eval(GetExpressionContext(), InstanceSampler->GetEnvironment()->GetIntParametersView(NameBlockPair.Value));
        }
        for (const auto& NameBlockPair : InstanceSampler->FactorGraph->GetParameterBlocks<float>())
        {
            if (!NameBlockPair.Key.ToString().EndsWith(TEXT(".Source"))) continue;
            FactorGraph->GetOutputs()[NameBlockPair.Key]->Eval(GetExpressionContext(), InstanceSampler->GetEnvironment()->GetFloatParametersView(NameBlockPair.Value));
        }
        InstanceSampler->SampleVariationSync();
        for (const auto& NameOutputPair : InstanceSampler->FactorGraph->GetOutputs())
        {
            const auto& Output = NameOutputPair.Value;
            const FName ParameterName = FName(InstanceNameSamplerPair.Key.ToString() + TEXT(".") + NameOutputPair.Key.ToString() + TEXT(".Target"));
            switch (Output->GetType())
            {
            case EConcordValueType::Int: Output->Eval(InstanceSampler->GetExpressionContext(), Environment->GetIntParametersView(FactorGraph->GetParameterBlocks<int32>()[ParameterName])); break;
            case EConcordValueType::Float: Output->Eval(InstanceSampler->GetExpressionContext(), Environment->GetFloatParametersView(FactorGraph->GetParameterBlocks<float>()[ParameterName])); break;
            default: checkNoEntry(); break;
            }
        }
    }
}

void FConcordSampler::SetColumnsFromOutputs(FConcordPatternData& OutPatternData) const
{
    TMap<FString, FConcordTrack> PreviousTracks = MoveTemp(OutPatternData.Tracks);
    OutPatternData.Tracks.Reset();
    for (const auto& NameOutputPair : GetFactorGraph()->GetOutputs())
    {
        if (NameOutputPair.Value->GetType() == EConcordValueType::Float) continue;
        const FString NameString = NameOutputPair.Key.ToString();
        if (NameString.EndsWith(TEXT(".Source"))) continue;
        const TOptional<FConcordColumnPath> OptionalColumnPath = FConcordColumnPath::Parse(NameString);
        if (!OptionalColumnPath) continue;
        const FConcordColumnPath& ColumnPath = OptionalColumnPath.GetValue();
        FConcordTrack& Track = OutPatternData.Tracks.FindOrAdd(FString(ColumnPath.TrackName));
        if (ColumnPath.ColumnIndex >= Track.Columns.Num()) Track.Columns.SetNum(ColumnPath.ColumnIndex + 1);
        FConcordColumn& Column = Track.Columns[ColumnPath.ColumnIndex];
        FConcordTrack* PreviousTrack = PreviousTracks.FindByHash(GetTypeHash(ColumnPath.TrackName), ColumnPath.TrackName);
        FConcordColumn* PreviousColumn = (PreviousTrack && ColumnPath.ColumnIndex < PreviousTrack->Columns.Num()) ? &PreviousTrack->Columns[ColumnPath.ColumnIndex] : nullptr;
        switch (ColumnPath.ColumnType)
        {
        case FConcordColumnPath::Note: SetColumnFromOutput(NameOutputPair.Value.Get(), Column.NoteValues, PreviousColumn ? &PreviousColumn->NoteValues : nullptr); break;
        case FConcordColumnPath::Instrument: SetColumnFromOutput(NameOutputPair.Value.Get(), Column.InstrumentValues, PreviousColumn ? &PreviousColumn->InstrumentValues : nullptr); break;
        case FConcordColumnPath::Volume: SetColumnFromOutput(NameOutputPair.Value.Get(), Column.VolumeValues, PreviousColumn ? &PreviousColumn->VolumeValues : nullptr); break;
        case FConcordColumnPath::Delay: SetColumnFromOutput(NameOutputPair.Value.Get(), Column.DelayValues, PreviousColumn ? &PreviousColumn->DelayValues : nullptr); break;
        }
    }
}

void FConcordSampler::FillCrateWithOutputs(FConcordCrateData& OutCrateData) const
{
    OutCrateData.IntBlocks.Reset();
    OutCrateData.FloatBlocks.Reset();
    for (const auto& NameOutputPair : GetFactorGraph()->GetOutputs())
    {
        const FString NameString = NameOutputPair.Key.ToString();
        if (NameString.EndsWith(TEXT(".Source"))) continue;
        switch (NameOutputPair.Value->GetType())
        {
        case EConcordValueType::Int:
            OutCrateData.IntBlocks.Add(NameOutputPair.Key).Values.SetNumUninitialized(NameOutputPair.Value->Num());
            NameOutputPair.Value->Eval(GetExpressionContext(), OutCrateData.IntBlocks[NameOutputPair.Key].Values);
            break;
        case EConcordValueType::Float:
            OutCrateData.FloatBlocks.Add(NameOutputPair.Key).Values.SetNumUninitialized(NameOutputPair.Value->Num());
            NameOutputPair.Value->Eval(GetExpressionContext(), OutCrateData.FloatBlocks[NameOutputPair.Key].Values);
            break;
        default: checkNoEntry(); break;
        }
    }
}

void FConcordSampler::SetColumnFromOutput(const FConcordFactorGraph<float>::FOutput* Output, TArray<int32>& TargetArray, TArray<int32>* PreviousArray) const
{
    if (PreviousArray) TargetArray = MoveTemp(*PreviousArray);
    TargetArray.SetNumUninitialized(Output->Num(), false);
    Output->Eval(GetExpressionContext(), TargetArray);
}

void FConcordSampler::GetVariationFromEnvironment()
{
    Variation = Environment->GetStagingVariation();
}

FConcordProbabilities FConcordSampler::GetConditionalProbabilities()
{
    FConcordProbabilities Probabilities; Probabilities.AddDefaulted(Variation.Num());
    TArray<float> Scores;
    for (int32 FlatRandomVariableIndex = 0; FlatRandomVariableIndex < Variation.Num(); ++FlatRandomVariableIndex)
        SamplingUtils.ComputeConditionalDistribution(FlatRandomVariableIndex, Scores, Probabilities[FlatRandomVariableIndex]);
    return MoveTemp(Probabilities);
}
