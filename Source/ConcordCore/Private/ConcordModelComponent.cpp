// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "ConcordModelComponent.h"
#include "ConcordNativeModel.h"
#include "Sampler/ConcordSampler.h"
#include "Sampler/ConcordExactSampler.h"

DEFINE_LOG_CATEGORY(LogConcordModelComponent);

UConcordModelComponent::UConcordModelComponent()
    : bRerunOnDoneAsync(false)
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UConcordModelComponent::SetIntParameter(FName ParameterName, int32 FlatParameterLocalIndex, int32 Value)
{
    if (!CheckSamplerExists() || !CheckParameterExists(ParameterName, EConcordValueType::Int)) return;
    if (!CheckFlatLocalIndex(Sampler->GetFactorGraph()->GetParameterBlocks<int32>()[ParameterName].Size, FlatParameterLocalIndex)) return;
    Sampler->GetEnvironment()->SetParameter(ParameterName, FlatParameterLocalIndex, Value);
}

void UConcordModelComponent::SetIntParameterArray(FName ParameterName, const TArray<int32>& Array)
{
    if (!CheckSamplerExists() || !CheckParameterExists(ParameterName, EConcordValueType::Int)) return;
    if (!CheckArrayNum(Sampler->GetFactorGraph()->GetParameterBlocks<int32>()[ParameterName].Size, Array.Num())) return;
    for (int32 FlatParameterLocalIndex = 0; FlatParameterLocalIndex < Array.Num(); ++FlatParameterLocalIndex)
        Sampler->GetEnvironment()->SetParameter(ParameterName, FlatParameterLocalIndex, Array[FlatParameterLocalIndex]);
}

int32 UConcordModelComponent::GetIntParameter(FName ParameterName, int32 FlatParameterLocalIndex) const
{
    if (!CheckSamplerExists() || !CheckParameterExists(ParameterName, EConcordValueType::Int)) return 0;
    if (!CheckFlatLocalIndex(Sampler->GetFactorGraph()->GetParameterBlocks<int32>()[ParameterName].Size, FlatParameterLocalIndex)) return 0;
    return Sampler->GetEnvironment()->GetParameter<int32>(ParameterName, FlatParameterLocalIndex);
}

void UConcordModelComponent::SetFloatParameter(FName ParameterName, int32 FlatParameterLocalIndex, float Value)
{
    if (!CheckSamplerExists() || !CheckParameterExists(ParameterName, EConcordValueType::Float)) return;
    if (!CheckFlatLocalIndex(Sampler->GetFactorGraph()->GetParameterBlocks<float>()[ParameterName].Size, FlatParameterLocalIndex)) return;
    Sampler->GetEnvironment()->SetParameter(ParameterName, FlatParameterLocalIndex, Value);
}

void UConcordModelComponent::SetFloatParameterArray(FName ParameterName, const TArray<float>& Array)
{
    if (!CheckSamplerExists() || !CheckParameterExists(ParameterName, EConcordValueType::Float)) return;
    if (!CheckArrayNum(Sampler->GetFactorGraph()->GetParameterBlocks<float>()[ParameterName].Size, Array.Num())) return;
    for (int32 FlatParameterLocalIndex = 0; FlatParameterLocalIndex < Array.Num(); ++FlatParameterLocalIndex)
        Sampler->GetEnvironment()->SetParameter(ParameterName, FlatParameterLocalIndex, Array[FlatParameterLocalIndex]);
}

float UConcordModelComponent::GetFloatParameter(FName ParameterName, int32 FlatParameterLocalIndex) const
{
    if (!CheckSamplerExists() || !CheckParameterExists(ParameterName, EConcordValueType::Float)) return 0;
    if (!CheckFlatLocalIndex(Sampler->GetFactorGraph()->GetParameterBlocks<float>()[ParameterName].Size, FlatParameterLocalIndex)) return 0;
    return Sampler->GetEnvironment()->GetParameter<float>(ParameterName, FlatParameterLocalIndex);
}

void UConcordModelComponent::ResetParameter(FName ParameterName)
{
    if (!CheckSamplerExists()) return;
    if (Sampler->GetFactorGraph()->GetParameterBlocks<int32>().Contains(ParameterName))
        Sampler->GetEnvironment()->ResetParameter<int32>(ParameterName);
    else if (Sampler->GetFactorGraph()->GetParameterBlocks<float>().Contains(ParameterName))
        Sampler->GetEnvironment()->ResetParameter<float>(ParameterName);
    else
        UE_LOG(LogConcordModelComponent, Error, TEXT("Trying to set a parameter that does not exist: %s in %s"), *ParameterName.ToString(), *GetName());
}

void UConcordModelComponent::ObserveValue(FName BoxName, int32 FlatBoxLocalIndex, int32 Value)
{
    if (!CheckSamplerExists() || !CheckBoxExists(BoxName)) return;
    if (!CheckFlatLocalIndex(Sampler->GetFactorGraph()->GetVariationBlocks()[BoxName].Size, FlatBoxLocalIndex)) return;
    Sampler->GetEnvironment()->ObserveValue(BoxName, FlatBoxLocalIndex, Value);
}

void UConcordModelComponent::ObserveArray(FName BoxName, const TArray<int32>& Array)
{
    if (!CheckSamplerExists() || !CheckBoxExists(BoxName)) return;
    if (!CheckArrayNum(Sampler->GetFactorGraph()->GetVariationBlocks()[BoxName].Size, Array.Num())) return;
    Sampler->GetEnvironment()->ObserveArray(BoxName, Array);
}

void UConcordModelComponent::ObserveRandomVariable(FName BoxName, int32 FlatBoxLocalIndex)
{
    if (!CheckSamplerExists() || !CheckBoxExists(BoxName)) return;
    if (!CheckFlatLocalIndex(Sampler->GetFactorGraph()->GetVariationBlocks()[BoxName].Size, FlatBoxLocalIndex)) return;
    Sampler->GetEnvironment()->Observe(BoxName, FlatBoxLocalIndex);
}

void UConcordModelComponent::ObserveBox(FName BoxName)
{
    if (!CheckSamplerExists() || !CheckBoxExists(BoxName)) return;
    Sampler->GetEnvironment()->Observe(BoxName);
}

void UConcordModelComponent::UnobserveRandomVariable(FName BoxName, int32 FlatBoxLocalIndex)
{
    if (!CheckSamplerExists() || !CheckBoxExists(BoxName)) return;
    if (!CheckFlatLocalIndex(Sampler->GetFactorGraph()->GetVariationBlocks()[BoxName].Size, FlatBoxLocalIndex)) return;
    Sampler->GetEnvironment()->Unobserve(BoxName, FlatBoxLocalIndex);
}

void UConcordModelComponent::UnobserveBox(FName BoxName)
{
    if (!CheckSamplerExists() || !CheckBoxExists(BoxName)) return;
    Sampler->GetEnvironment()->Unobserve(BoxName);
}

void UConcordModelComponent::SetCrate(const FConcordCrateData& CrateData)
{
    if (!CheckSamplerExists()) return;
    Sampler->GetEnvironment()->SetCrate(CrateData);
}

void UConcordModelComponent::SetNamedCrate(FName Name, const FConcordCrateData& CrateData)
{
    if (!CheckSamplerExists()) return;
    TArray<FName>& BlockNames = NamedCrateBlockNames.FindOrAdd(Name);
    for (const FName& BlockName : BlockNames) Sampler->GetEnvironment()->UnsetName(BlockName);
    BlockNames.Reset();
    Sampler->GetEnvironment()->SetCrate(CrateData);
    for (const auto& NameBlockPair : CrateData.IntBlocks) BlockNames.Add(NameBlockPair.Key);
    for (const auto& NameBlockPair : CrateData.FloatBlocks) BlockNames.Add(NameBlockPair.Key);
}

void UConcordModelComponent::UnsetCrate(const FConcordCrateData& CrateData)
{
    if (!CheckSamplerExists()) return;
    Sampler->GetEnvironment()->UnsetCrate(CrateData);
}

void UConcordModelComponent::RunSamplerAsync(const FOnVariationSampled& InOnVariationSampled)
{
    if (!CheckSamplerExists()) return;
    OnVariationSampled = InOnVariationSampled;
    if (Sampler->IsSamplingVariation())
    {
        bRerunOnDoneAsync = true;
        UE_LOG(LogConcordModelComponent, Warning, TEXT("Requesting a sampling while another async sampling is in progress. The result of the current sampling will be discarded."));
        return;
    }
    Sampler->GetEnvironment()->SetMaskAndParametersFromStagingArea();
    Sampler->GetVariationFromEnvironment();
    Sampler->SampleVariationAsync();
}

void UConcordModelComponent::RunSamplerSync(float& Score)
{
    if (!CheckSamplerExists()) return;
    Sampler->GetEnvironment()->SetMaskAndParametersFromStagingArea();
    Sampler->GetVariationFromEnvironment();
    Score = Sampler->SampleVariationSync();
    Sampler->GetEnvironment()->ReturnSampledVariationToStagingArea(Sampler->GetVariation());
}

void UConcordModelComponent::GetOutput(FName OutputName, TArray<int32>& Array)
{
    if (!CheckSamplerExists() || !CheckOutputExists(OutputName, EConcordValueType::Int)) return;
    const auto& Output = Sampler->GetFactorGraph()->GetOutputs()[OutputName];
    Array.SetNumUninitialized(Output->Num());
    Output->Eval(Sampler->GetExpressionContext(), Array);
}

void UConcordModelComponent::GetOutputOverwrite(FName OutputName, UPARAM(ref) TArray<int32>& InArray, TArray<int32>& Array)
{
    Array = InArray;
    GetOutput(OutputName, Array);
}

void UConcordModelComponent::GetOutputOverwriteParameter(FName OutputName, UPARAM(ref) UConcordModelComponent*& ComponentWithParameter, FName ParameterName)
{
    if (!CheckSamplerExists() || !ComponentWithParameter->CheckSamplerExists() || !CheckOutputExists(OutputName, EConcordValueType::Int)) return;
    const auto& Output = Sampler->GetFactorGraph()->GetOutputs()[OutputName];
    FConcordFactorGraphBlock Block;
    const TOptional<EConcordValueType> OptionalType = ComponentWithParameter->CheckParameterExists(ParameterName, &Block);
    if (!OptionalType) return;
    switch (OptionalType.GetValue())
    {
    case EConcordValueType::Int:
    {
        const TArrayView<int32> ParametersView = ComponentWithParameter->GetSampler()->GetEnvironment()->GetStagingIntParametersView(Block);
        Output->Eval(Sampler->GetExpressionContext(), ParametersView);
        return;
    }
    case EConcordValueType::Float:
    {
        const TArrayView<float> ParametersView = ComponentWithParameter->GetSampler()->GetEnvironment()->GetStagingFloatParametersView(Block);
        Output->Eval(Sampler->GetExpressionContext(), ParametersView);
        return;
    }
    default: checkNoEntry(); return;
    }
}

void UConcordModelComponent::OutputPattern(UConcordPattern*& Pattern)
{
    Pattern = NewObject<UConcordPattern>(this);
    OverwritePattern(Pattern, Pattern);
}

void UConcordModelComponent::OverwritePattern(UPARAM(ref) UConcordPattern*& Pattern, UConcordPattern*& OutPattern)
{
    if (!CheckSamplerExists()) return;
    if (!Pattern)
    {
        UE_LOG(LogConcordModelComponent, Log, TEXT("Pattern to overwrite was nullptr, creating new pattern."));
        Pattern = NewObject<UConcordPattern>(this);
    }
    Sampler->SetColumnsFromOutputs(Pattern->PatternData);
    OutPattern = Pattern;
}

void UConcordModelComponent::OutputPatternData(FConcordPatternData& PatternData)
{
    if (!CheckSamplerExists()) return;
    Sampler->SetColumnsFromOutputs(PatternData);
}

void UConcordModelComponent::OutputCrateData(FConcordCrateData& CrateData)
{
    if (!CheckSamplerExists()) return;
    Sampler->FillCrateWithOutputs(CrateData);
}

void UConcordModelComponent::GetFloatOutput(FName OutputName, TArray<float>& Array)
{
    if (!CheckSamplerExists() || !CheckOutputExists(OutputName, EConcordValueType::Float)) return;
    const auto& Output = Sampler->GetFactorGraph()->GetOutputs()[OutputName];
    Array.SetNumUninitialized(Output->Num());
    Output->Eval(Sampler->GetExpressionContext(), Array);
}

void UConcordModelComponent::GetFloatOutputOverwrite(FName OutputName, UPARAM(ref) TArray<float>& InArray, TArray<float>& Array)
{
    Array = InArray;
    GetFloatOutput(OutputName, Array);
}

void UConcordModelComponent::BeginPlay()
{
    Setup();
    Super::BeginPlay();
}

void UConcordModelComponent::Setup()
{
    if (!Model)
    {
        UE_LOG(LogConcordModelComponent, Warning, TEXT("No Concord model set in %s."), *GetName());
        return;
    }
    TOptional<FConcordError> ModelError;
    TSharedPtr<const FConcordFactorGraph<float>> FactorGraph = Model->GetFactorGraph(GetActiveSamplerFactory()->GetCycleMode(), ModelError);
    if (ModelError)
    {
        UE_LOG(LogConcordModelComponent, Error, TEXT("Error building factor graph in %s: %s"), *GetName(), *ModelError.GetValue().Message);
        return;
    }
    TOptional<FString> SamplerErrorMessage;
    Sampler = GetActiveSamplerFactory()->CreateSampler(FactorGraph.ToSharedRef(), SamplerErrorMessage);
    if (SamplerErrorMessage)
    {
        UE_LOG(LogConcordModelComponent, Error, TEXT("Error building sampler in %s: %s"), *GetName(), *SamplerErrorMessage.GetValue());
    }
    else
    {
        for (const UConcordCrate* Crate : Model->DefaultCrates) if (Crate) Sampler->GetEnvironment()->SetCrate(Crate->CrateData);
        for (const UConcordCrate* Crate : ComponentCrates) if (Crate) Sampler->GetEnvironment()->SetCrate(Crate->CrateData);
    }
}

const UConcordSamplerFactory* UConcordModelComponent::GetActiveSamplerFactory() const
{
    const UConcordSamplerFactory* SamplerFactory = SamplerFactoryOverride ? SamplerFactoryOverride : Model->DefaultSamplerFactory;
    if (!SamplerFactory) SamplerFactory = NewObject<UConcordExactSamplerFactory>();
    return SamplerFactory;
}

void UConcordModelComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!Sampler) return;
    if (TOptional<float> Score = Sampler->GetScoreIfDoneSampling())
    {
        if (bRerunOnDoneAsync)
        {
            bRerunOnDoneAsync = false;
            Sampler->GetEnvironment()->SetMaskAndParametersFromStagingArea();
            Sampler->GetVariationFromEnvironment();
            Sampler->SampleVariationAsync();
            return;
        }
        Sampler->GetEnvironment()->ReturnSampledVariationToStagingArea(Sampler->GetVariation());
        OnVariationSampledNative.ExecuteIfBound(Score.GetValue());
        OnVariationSampled.ExecuteIfBound(Score.GetValue());
    }
}

bool UConcordModelComponent::CheckSamplerExists() const
{
    if (!Sampler)
    {
        UE_LOG(LogConcordModelComponent, Warning, TEXT("%s does not have a valid sampler object."), *GetName());
        return false;
    }
    return true;
}

bool UConcordModelComponent::CheckBoxExists(const FName& Name) const
{
    if (!Sampler->GetFactorGraph()->GetVariationBlocks().Contains(Name))
    {
        UE_LOG(LogConcordModelComponent, Error, TEXT("Trying to make observations for a box that does not exist: %s in %s"), *Name.ToString(), *GetName());
        return false;
    }
    return true;
}

bool UConcordModelComponent::CheckParameterExists(const FName& Name, EConcordValueType Type) const
{
    bool bExists = false;
    switch (Type)
    {
    case EConcordValueType::Int: bExists = Sampler->GetFactorGraph()->GetParameterBlocks<int32>().Contains(Name); break;
    case EConcordValueType::Float: bExists = Sampler->GetFactorGraph()->GetParameterBlocks<float>().Contains(Name); break;
    default: checkNoEntry();
    }
    if (!bExists) UE_LOG(LogConcordModelComponent, Error, TEXT("Trying to set a parameter that does not exist for the required type: %s in %s"), *Name.ToString(), *GetName());
    return bExists;
}

bool UConcordModelComponent::CheckArrayNum(int32 BlockSize, int32 Num) const
{
    if (BlockSize != Num)
    {
        UE_LOG(LogConcordModelComponent, Error, TEXT("Trying to set array with invalid length %i (expected %i) in %s."), Num, BlockSize, *GetName());
        return false;
    }
    return true;
}

bool UConcordModelComponent::CheckFlatLocalIndex(int32 BlockSize, int32 FlatLocalIndex) const
{
    if (FlatLocalIndex < 0 || FlatLocalIndex >= BlockSize)
    {
        UE_LOG(LogConcordModelComponent, Error, TEXT("Trying to set a value with an invalid index %i (needs to be < %i) in %s."), FlatLocalIndex, BlockSize, *GetName());
        return false;
    }
    return true;
}

bool UConcordModelComponent::CheckOutputExists(const FName& Name, EConcordValueType Type) const
{
    if (!Sampler->GetFactorGraph()->GetOutputs().Contains(Name) || Sampler->GetFactorGraph()->GetOutputs()[Name]->GetType() != Type)
    {
        UE_LOG(LogConcordModelComponent, Error, TEXT("Trying to read an output that does not exist for the required type: %s in %s"), *Name.ToString(), *GetName());
        return false;
    }
    return true;
}

TOptional<EConcordValueType> UConcordModelComponent::CheckParameterExists(const FName& Name, FConcordFactorGraphBlock* OutBlock) const
{
    if (auto FoundBlock = Sampler->GetFactorGraph()->GetParameterBlocks<int32>().Find(Name))
    {
        *OutBlock = *FoundBlock;
        return EConcordValueType::Int;
    }
    if (auto FoundBlock = Sampler->GetFactorGraph()->GetParameterBlocks<float>().Find(Name))
    {
        *OutBlock = *FoundBlock;
        return EConcordValueType::Float;
    }
    UE_LOG(LogConcordModelComponent, Error, TEXT("Trying to set a parameter %s that does not exist in %s."), *Name.ToString(), *GetName());
    return {};
}
