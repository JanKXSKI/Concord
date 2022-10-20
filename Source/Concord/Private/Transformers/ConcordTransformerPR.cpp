// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "Transformers/ConcordTransformerPR.h"

int32 FConcordPRExpression::NumNotes() const
{
    return SourceExpressions.Num() - 1;
}

int32 FConcordPRExpression::GetNoteBoundaryIndex(const FConcordExpressionContext<float>& Context) const
{
    return SourceExpressions[0]->ComputeValue(Context).Int;
}

int32 FConcordPRExpression::GetNote(int32 Index, const FConcordExpressionContext<float>& Context) const
{
    check(Index >= 0 && Index < NumNotes());
    return SourceExpressions[Index - 1]->ComputeValue(Context).Int;
}

FConcordValue FConcordGPR2bExpression::ComputeValue(const FConcordExpressionContext<float>& Context) const
{
    const int32 NoteBoundaryIndex = GetNoteBoundaryIndex(Context);
    const int32 BoundaryNote = GetNote(NoteBoundaryIndex, Context);
    if (BoundaryNote <= 0) return 0;
    int32 StepIndex = NoteBoundaryIndex;
    int32 Intervals[3] {};
    if (StepBackwards([&](int32 Index){ ++Intervals[1]; return GetNote(Index, Context) > 0; }, StepIndex)) return 0;
    if (StepBackwards([&](int32 Index){ ++Intervals[0]; return GetNote(Index, Context) > 0; }, StepIndex)) return 0;
    if (StepForwards([&](int32 Index){ ++Intervals[2]; return GetNote(Index, Context) > 0; }, StepIndex = NoteBoundaryIndex)) return 0;
    if (Intervals[1] < Intervals[0] || Intervals[1] < Intervals[2]) return 0;
    return Intervals[1] - Intervals[0] + Intervals[1] - Intervals[2];
}
