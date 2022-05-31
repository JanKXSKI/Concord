// Copyright Jan Klimaschewski. All Rights Reserved.

#include "FactorGraph/ConcordFactorGraph.h"

template<typename FFloatType>
TArray<TPair<FName, FConcordFactorGraphBlock>> FConcordFactorGraph<FFloatType>::GetAllParameterBlocks() const
{
    TArray<TPair<FName, FConcordFactorGraphBlock>> AllParameterBlocks;
    for (const auto& NameBlockPair : GetParameterBlocks<int32>()) AllParameterBlocks.Add(NameBlockPair);
    for (const auto& NameBlockPair : GetParameterBlocks<float>()) AllParameterBlocks.Add(NameBlockPair);
    return MoveTemp(AllParameterBlocks);
}

template<typename FFloatType>
uint64 FConcordFactorGraph<FFloatType>::GetComplexity() const
{
    uint64 Complexity = 0;
    for (const auto& Handle : GetHandles())
    {
        uint64 HandleComplexity = 1;
        for (int32 FlatRandomVariableIndex : Handle->GetNeighboringFlatRandomVariableIndices())
        {
            const int32 StateCount = GetStateCount(FlatRandomVariableIndex);
            if (HandleComplexity > MAX_uint64 / StateCount) return MAX_uint64;
            HandleComplexity *= StateCount;
        }
        if (Complexity > MAX_uint64 - HandleComplexity) return MAX_uint64;
        Complexity += HandleComplexity;
    }
    return Complexity;
}

template<typename FFloatType>
void FConcordFactorGraph<FFloatType>::CompactAndShrink()
{
    Handles.Shrink();
    RandomVariableStateCounts.Shrink();
    RandomVariableNeighboringHandles.Shrink();
    for (auto& NeighboringHandles : RandomVariableNeighboringHandles) NeighboringHandles.Shrink();
    DisjointSubgraphRootFlatRandomVariableIndices.Shrink();
    Outputs.CompactStable();
    Outputs.Shrink();

    VariationBlocks.CompactStable();
    VariationBlocks.Shrink();
    IntParameterBlocks.CompactStable();
    IntParameterBlocks.Shrink();
    FloatParameterBlocks.CompactStable();
    FloatParameterBlocks.Shrink();
    IntParameterDefaultValues.Shrink();
    FloatParameterDefaultValues.Shrink();
}

template CONCORDCORE_API class FConcordFactorGraph<float>;
