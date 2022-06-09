// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordFactorGraph.h"
#include "ConcordCrate.h"

DECLARE_LOG_CATEGORY_EXTERN(LogConcordFactorGraphEnvironment, Log, All);

template<typename FFloatType>
class CONCORDCORE_API FConcordFactorGraphEnvironment
{
public:
    FConcordFactorGraphEnvironment(const TSharedRef<const FConcordFactorGraph<FFloatType>>& InFactorGraph);

    void SetMaskAndParametersFromStagingArea();
    void ReturnSampledVariationToStagingArea(const TArray<int32>& Variation);

    template<typename FValue> void SetParameter(const FName& ParameterName, int32 FlatParameterLocalIndex, FValue Value);
    template<typename FValue> FValue GetParameter(const FName& ParameterName, int32 FlatParameterLocalIndex) const;
    template<typename FValue> void ResetParameter(const FName& ParameterName);

    void ObserveValue(const FName& BoxName, int32 FlatBoxLocalIndex, int32 Value);
    void ObserveArray(const FName& BoxName, const TArray<int32>& Values);
    void Observe(const FName& BoxName, int32 FlatBoxLocalIndex);
    void Observe(const FName& BoxName);
    void Unobserve(const FName& BoxName, int32 FlatBoxLocalIndex);
    void Unobserve(const FName& BoxName);

    void SetCrate(const FConcordCrateData& CrateData);
    void UnsetCrate(const FConcordCrateData& CrateData);
    void UnsetName(const FName& Name);

    const FConcordVariation& GetStagingVariation() const { return StagingVariation; }
    FConcordVariation& GetMutableStagingVariation() { return StagingVariation; } // used in training when staging is not neccessary
    const FConcordObservationMask& GetStagingMask() const { return StagingMask; }
    const TArray<int32>& GetStagingIntParameters() const { return StagingIntParameters; }
    const TArray<FFloatType>& GetStagingFloatParameters() const { return StagingFloatParameters; }
    TArray<FFloatType>& GetMutableStagingFloatParameters() { return StagingFloatParameters; } // used in training
    TArrayView<int32> GetStagingIntParametersView(const FConcordFactorGraphBlock& Block) { return MakeArrayView(StagingIntParameters.GetData() + Block.Offset, Block.Size); }
    TArrayView<FFloatType> GetStagingFloatParametersView(const FConcordFactorGraphBlock& Block) { return MakeArrayView(StagingFloatParameters.GetData() + Block.Offset, Block.Size); }

    const FConcordObservationMask& GetMask() const { return Mask; }
    const TArray<int32>& GetIntParameters() const { return IntParameters; }
    const TArray<FFloatType>& GetFloatParameters() const { return FloatParameters; }
    TArrayView<int32> GetIntParametersView(const FConcordFactorGraphBlock& Block) { return MakeArrayView(IntParameters.GetData() + Block.Offset, Block.Size); }
    TArrayView<FFloatType> GetFloatParametersView(const FConcordFactorGraphBlock& Block) { return MakeArrayView(FloatParameters.GetData() + Block.Offset, Block.Size); }
private:
    template<typename FValue> bool TrySetParameterBlock(const FName& BlockName, const TArray<FValue>& Values);
    template<typename FValue> bool TryUnsetParameterBlock(const FName& BlockName);

    template<typename FValue> TArray<FValue>& GetStagingParameters();
    template<> inline TArray<int32>& GetStagingParameters<int32>() { return StagingIntParameters; }
    template<> inline TArray<FFloatType>& GetStagingParameters<FFloatType>() { return StagingFloatParameters; }
    template<typename FValue> const TArray<FValue>& GetStagingParameters() const;
    template<> inline const TArray<int32>& GetStagingParameters<int32>() const { return StagingIntParameters; }
    template<> inline const TArray<FFloatType>& GetStagingParameters<FFloatType>() const { return StagingFloatParameters; }

    TSharedRef<const FConcordFactorGraph<FFloatType>> FactorGraph;

    FConcordVariation StagingVariation;
    FConcordObservationMask StagingMask;
    TArray<int32> StagingIntParameters;
    TArray<FFloatType> StagingFloatParameters;

    FConcordObservationMask Mask;
    TArray<int32> IntParameters;
    TArray<FFloatType> FloatParameters;
};
