// Copyright Jan Klimaschewski. All Rights Reserved.

#include "FactorGraph/ConcordFactorGraphEnvironment.h"

DEFINE_LOG_CATEGORY(LogConcordFactorGraphEnvironment);

template<typename FFloatType>
FConcordFactorGraphEnvironment<FFloatType>::FConcordFactorGraphEnvironment(const TSharedRef<const FConcordFactorGraph<FFloatType>>& InFactorGraph)
    : FactorGraph(InFactorGraph)
{
    StagingVariation.Init(0, FactorGraph->GetRandomVariableCount());
    StagingMask.Init(false, FactorGraph->GetRandomVariableCount());
    StagingIntParameters = FactorGraph->GetParameterDefaultValues<int32>();
    StagingFloatParameters = MakeArrayView(FactorGraph->GetParameterDefaultValues<float>());
    SetMaskAndParametersFromStagingArea();
}

template<typename FFloatType>
void FConcordFactorGraphEnvironment<FFloatType>::SetMaskAndParametersFromStagingArea()
{
    Mask = StagingMask;
    IntParameters = StagingIntParameters;
    FloatParameters = StagingFloatParameters;
}

template<typename FFloatType>
void FConcordFactorGraphEnvironment<FFloatType>::ReturnSampledVariationToStagingArea(const TArray<int32>& Variation)
{
    for (int32 FlatRandomVariableIndex = 0; FlatRandomVariableIndex < StagingVariation.Num(); ++FlatRandomVariableIndex)
        if (!StagingMask[FlatRandomVariableIndex]) StagingVariation[FlatRandomVariableIndex] = Variation[FlatRandomVariableIndex];
}

template<typename FFloatType>
template<typename FValue> void FConcordFactorGraphEnvironment<FFloatType>::SetParameter(const FName& ParameterName, int32 FlatParameterLocalIndex, FValue Value)
{
    GetStagingParameters<FValue>()[FactorGraph->GetParameterBlocks<FValue>()[ParameterName].Offset + FlatParameterLocalIndex] = Value;
}

template<typename FFloatType>
template<typename FValue> FValue FConcordFactorGraphEnvironment<FFloatType>::GetParameter(const FName& ParameterName, int32 FlatParameterLocalIndex) const
{
    return GetStagingParameters<FValue>()[FactorGraph->GetParameterBlocks<FValue>()[ParameterName].Offset + FlatParameterLocalIndex];
}

template<typename FValue> struct FDefaultValue { using type = float; };
template<> struct FDefaultValue<int32> { using type = int32; };

template<typename FFloatType>
template<typename FValue> void FConcordFactorGraphEnvironment<FFloatType>::ResetParameter(const FName& ParameterName)
{
    const FConcordFactorGraphBlock& Block = FactorGraph->GetParameterBlocks<FValue>()[ParameterName];
    for (int32 Index = Block.Offset; Index < Block.Offset + Block.Size; ++Index)
        GetStagingParameters<FValue>()[Index] = FactorGraph->GetParameterDefaultValues<typename FDefaultValue<FValue>::type>()[Index];
}

template<typename FFloatType>
void FConcordFactorGraphEnvironment<FFloatType>::ObserveValue(const FName& BoxName, int32 FlatBoxLocalIndex, int32 Value)
{
    const int32 FlatIndex = FactorGraph->GetVariationBlocks()[BoxName].Offset + FlatBoxLocalIndex;
    if (Value < 0 || Value >= FactorGraph->GetStateCount(FlatIndex))
    {
        UE_LOG(LogConcordFactorGraphEnvironment, Error, TEXT("Skipping observation for box %s, 0 <= %i < StateCount does not hold."), *BoxName.ToString(), Value);
        return;
    }
    StagingMask[FlatIndex] = true;
    StagingVariation[FlatIndex] = Value;
}

template<typename FFloatType>
void FConcordFactorGraphEnvironment<FFloatType>::ObserveArray(const FName& BoxName, const TArray<int32>& Values)
{
    for (int32 FlatBoxLocalIndex = 0; FlatBoxLocalIndex < FactorGraph->GetVariationBlocks()[BoxName].Size; ++FlatBoxLocalIndex)
        ObserveValue(BoxName, FlatBoxLocalIndex, Values[FlatBoxLocalIndex]);
}

template<typename FFloatType>
void FConcordFactorGraphEnvironment<FFloatType>::Observe(const FName& BoxName, int32 FlatBoxLocalIndex)
{
    StagingMask[FactorGraph->GetVariationBlocks()[BoxName].Offset + FlatBoxLocalIndex] = true;
}

template<typename FFloatType>
void FConcordFactorGraphEnvironment<FFloatType>::Observe(const FName& BoxName)
{
    const FConcordFactorGraphBlock& VariationBlock = FactorGraph->GetVariationBlocks()[BoxName];
    for (int32 FlatIndex = VariationBlock.Offset; FlatIndex < VariationBlock.Offset + VariationBlock.Size; ++FlatIndex)
        StagingMask[FlatIndex] = true;
}

template<typename FFloatType>
void FConcordFactorGraphEnvironment<FFloatType>::Unobserve(const FName& BoxName, int32 FlatBoxLocalIndex)
{
    StagingMask[FactorGraph->GetVariationBlocks()[BoxName].Offset + FlatBoxLocalIndex] = false;
}

template<typename FFloatType>
void FConcordFactorGraphEnvironment<FFloatType>::Unobserve(const FName& BoxName)
{
    const FConcordFactorGraphBlock& VariationBlock = FactorGraph->GetVariationBlocks()[BoxName];
    for (int32 FlatIndex = VariationBlock.Offset; FlatIndex < VariationBlock.Offset + VariationBlock.Size; ++FlatIndex)
        StagingMask[FlatIndex] = false;
}

template<typename FFloatType>
void FConcordFactorGraphEnvironment<FFloatType>::SetCrate(const FConcordCrateData& CrateData)
{
    for (const TPair<FName, FConcordIntBlock>& NameIntBlockPair : CrateData.IntBlocks)
    {
        const TArray<int32>& Values = NameIntBlockPair.Value.Values;
        if (const FConcordFactorGraphBlock* VariationBlock = FactorGraph->GetVariationBlocks().Find(NameIntBlockPair.Key))
        {
            if (Values.Num() != VariationBlock->Size) UE_LOG(LogConcordFactorGraphEnvironment, Warning, TEXT("Block %s is not of the expected size, some values are not set."), *NameIntBlockPair.Key.ToString());
            for (int32 Index = 0; Index < FMath::Min(Values.Num(), VariationBlock->Size); ++Index) ObserveValue(NameIntBlockPair.Key, Index, Values[Index]);
        }
        else if (!TrySetParameterBlock(NameIntBlockPair.Key, Values))
        {
            UE_LOG(LogConcordFactorGraphEnvironment, Warning, TEXT("Skipping Block in call to SetCrate, %s is not a box or parameter of integer type in the model."), *NameIntBlockPair.Key.ToString());
        }
    }
    for (const TPair<FName, FConcordFloatBlock>& NameFloatBlockPair : CrateData.FloatBlocks)
        if (!TrySetParameterBlock(NameFloatBlockPair.Key, NameFloatBlockPair.Value.Values))
            UE_LOG(LogConcordFactorGraphEnvironment, Warning, TEXT("Skipping Block in call to SetCrate, %s is not a parameter of float type in the model."), *NameFloatBlockPair.Key.ToString());
}

template<typename FEnvFloat, typename FArg> struct FBlockType { using type = FArg; };
template<typename FEnvFloat> struct FBlockType<FEnvFloat, float> { using type = FEnvFloat; };
template<typename FEnvFloat, typename FArg> using FBlockType_t = typename FBlockType<FEnvFloat, FArg>::type;

template<typename FFloatType>
template<typename FValue> bool FConcordFactorGraphEnvironment<FFloatType>::TrySetParameterBlock(const FName& BlockName, const TArray<FValue>& Values)
{
    const FConcordFactorGraphBlock* ParameterBlock = FactorGraph->GetParameterBlocks<FBlockType_t<FFloatType, FValue>>().Find(BlockName);
    if (!ParameterBlock) return false;
    if (Values.Num() != ParameterBlock->Size) UE_LOG(LogConcordFactorGraphEnvironment, Warning, TEXT("Block %s is not of the expected size, some values are not set."), *BlockName.ToString());
    for (int32 Index = 0; Index < FMath::Min(Values.Num(), ParameterBlock->Size); ++Index) SetParameter<FBlockType_t<FFloatType, FValue>>(BlockName, Index, Values[Index]);
    return true;
}

template<typename FFloatType>
void FConcordFactorGraphEnvironment<FFloatType>::UnsetCrate(const FConcordCrateData& CrateData)
{
    for (const auto& NameBlockPair : CrateData.IntBlocks) UnsetName(NameBlockPair.Key);
    for (const auto& NameBlockPair : CrateData.FloatBlocks) UnsetName(NameBlockPair.Key);
}

template<typename FFloatType>
void FConcordFactorGraphEnvironment<FFloatType>::UnsetName(const FName& Name)
{
    if (const FConcordFactorGraphBlock* VariationBlock = FactorGraph->GetVariationBlocks().Find(Name)) Unobserve(Name);
    else if (!TryUnsetParameterBlock<int32>(Name) && !TryUnsetParameterBlock<FFloatType>(Name))
        UE_LOG(LogConcordFactorGraphEnvironment, Warning, TEXT("Cannot unset name, %s is not a box or parameter name in the model."), *Name.ToString());
}

template<typename FFloatType>
template<typename FValue> bool FConcordFactorGraphEnvironment<FFloatType>::TryUnsetParameterBlock(const FName& BlockName)
{
    const FConcordFactorGraphBlock* ParameterBlock = FactorGraph->GetParameterBlocks<FValue>().Find(BlockName);
    if (!ParameterBlock) return false;
    ResetParameter<FValue>(BlockName);
    return true;
}

template CONCORDCORE_API class FConcordFactorGraphEnvironment<float>;
template CONCORDCORE_API void FConcordFactorGraphEnvironment<float>::SetParameter(const FName&, int32, int32);
template CONCORDCORE_API void FConcordFactorGraphEnvironment<float>::SetParameter(const FName&, int32, float);
template CONCORDCORE_API int32 FConcordFactorGraphEnvironment<float>::GetParameter<int32>(const FName&, int32) const;
template CONCORDCORE_API float FConcordFactorGraphEnvironment<float>::GetParameter<float>(const FName&, int32) const;
template CONCORDCORE_API void FConcordFactorGraphEnvironment<float>::ResetParameter<int32>(const FName&);
template CONCORDCORE_API void FConcordFactorGraphEnvironment<float>::ResetParameter<float>(const FName&);
