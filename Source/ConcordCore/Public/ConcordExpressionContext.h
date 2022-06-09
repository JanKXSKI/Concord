// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

using FConcordVariation = TArray<int32>;
using FConcordObservationMask = TArray<bool>;
using FConcordDistribution = TArray<float>;
using FConcordProbabilities = TArray<FConcordDistribution>;

template<typename FFloatType>
struct FConcordExpressionContextMutable
{
    FConcordVariation& Variation;
    const FConcordObservationMask& ObservationMask;
    const TArray<int32>& IntParameters;
    const TArray<FFloatType>& FloatParameters;
};

template<typename FFloatType>
struct FConcordExpressionContext
{
    FConcordExpressionContext(const FConcordVariation& InVariation, const FConcordObservationMask& InObservationMask, const TArray<int32>& InIntParameters, const TArray<FFloatType>& InFloatParameters)
        : Variation(InVariation), ObservationMask(InObservationMask), IntParameters(InIntParameters), FloatParameters(InFloatParameters) {}
    FConcordExpressionContext(const FConcordExpressionContextMutable<FFloatType>& MutableContext)
        : FConcordExpressionContext(MutableContext.Variation, MutableContext.ObservationMask, MutableContext.IntParameters, MutableContext.FloatParameters) {}
    const FConcordVariation& Variation;
    const FConcordObservationMask& ObservationMask;
    const TArray<int32>& IntParameters;
    const TArray<FFloatType>& FloatParameters;
};
