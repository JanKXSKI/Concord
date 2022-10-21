// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "Transformers/ConcordTransformerPR.h"

int32 FConcordGPR1Expression::ComputeScoreClass(const FConcordExpressionContext<float>& Context) const
{
    int32 NumNotes = 0;
    const auto F = [&](int32 Degree){ if (Degree > 0) ++NumNotes; return NumNotes > 2; };
    if (int32 Index = 0; StepForward(F, Index, Context)) return 0;
    switch (NumNotes)
    {
    case 0: return -4;
    case 1: return -2;
    default: return -1;
    }
}

int32 FConcordGPR2bExpression::ComputeScoreClass(const FConcordExpressionContext<float>& Context) const
{
    const int32 BoundaryIndex = GetBoundaryIndex(Context);
    const int32 BoundaryDegree = GetDegree(BoundaryIndex, Context);
    if (BoundaryDegree <= 0) return 0;
    int32 StepIndex = BoundaryIndex;
    int32 Intervals[3] {};
    if (StepBackward([&](int32 Degree){ ++Intervals[1]; return Degree <= 0; }, StepIndex, Context)) return 0;
    if (StepBackward([&](int32 Degree){ ++Intervals[0]; return Degree <= 0; }, StepIndex, Context)) return 0;
    if (StepForward([&](int32 Degree){ ++Intervals[2]; return Degree <= 0; }, StepIndex = BoundaryIndex, Context)) return 0;
    if (Intervals[1] <= Intervals[0] || Intervals[1] <= Intervals[2]) return 0;
    return Intervals[1] - Intervals[0] + Intervals[1] - Intervals[2];
}
