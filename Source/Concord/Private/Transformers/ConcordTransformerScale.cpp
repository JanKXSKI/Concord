// Copyright Jan Klimaschewski. All Rights Reserved.

#include "Transformers/ConcordTransformerScale.h"

FConcordToDegreeExpression::FConcordToDegreeExpression(TArray<FConcordSharedExpression>&& InSourceExpressions)
    : FConcordComputingExpression(MoveTemp(InSourceExpressions))
{}

FConcordValue FConcordToDegreeExpression::ComputeValue(const FConcordExpressionContext<float>& Context) const
{
    const int32 Root = SourceExpressions[0]->ComputeValue(Context).Int;
    const int32 Note = SourceExpressions[1]->ComputeValue(Context).Int;
    int32 SemitoneOffset = Note - Root;
    int32 Octave;
    if (SemitoneOffset < 0)
    {
        Octave = (SemitoneOffset + 1) / 12 - 1;
        SemitoneOffset = 12 + (SemitoneOffset + 1) % 12 - 1;
    }
    else
    {
        Octave = SemitoneOffset / 12;
        SemitoneOffset = SemitoneOffset % 12;
    }
    for (int32 ScaleDegree = 1; ScaleDegree <= SourceExpressions.Num() - 3; ++ScaleDegree)
        if (SourceExpressions[3 + ScaleDegree - 1]->ComputeValue(Context).Int == SemitoneOffset)
            return Octave * (SourceExpressions.Num() - 3) + ScaleDegree;
    return SourceExpressions[2]->ComputeValue(Context).Int;
}

#if WITH_EDITOR
FString FConcordToDegreeExpression::ToString() const
{
    return FString::Format(TEXT(R"Expression(
        const int32 Root = $int0;
        const int32 Note = $int1;
        int32 SemitoneOffset = Note - Root;
        int32 Octave;
        if (SemitoneOffset < 0)
        {
            Octave = (SemitoneOffset + 1) / 12 - 1;
            SemitoneOffset = 12 + (SemitoneOffset + 1) %% 12 - 1;
        }
        else
        {
            Octave = SemitoneOffset / 12;
            SemitoneOffset = SemitoneOffset %% 12;
        }
        for (int32 ScaleDegree = 1; ScaleDegree <= {0}; ++ScaleDegree)
            if ($int3array(ScaleDegree - 1) == SemitoneOffset)
                return Octave * {0} + ScaleDegree;
        return $int2;
    )Expression"), { SourceExpressions.Num() - 3 });
}
#endif

FConcordToNoteExpression::FConcordToNoteExpression(TArray<FConcordSharedExpression>&& InSourceExpressions)
    : FConcordComputingExpression(MoveTemp(InSourceExpressions))
{}

FConcordValue FConcordToNoteExpression::ComputeValue(const FConcordExpressionContext<float>& Context) const
{
    const int32 Root = SourceExpressions[0]->ComputeValue(Context).Int;
    int32 Degree = SourceExpressions[1]->ComputeValue(Context).Int;
    const int32 ScaleSemitoneOffsetsNum = SourceExpressions.Num() - 2;
    int32 Octave;
    if (Degree <= 0)
    {
        Octave = Degree / ScaleSemitoneOffsetsNum - 1;
        Degree = ScaleSemitoneOffsetsNum + Degree % ScaleSemitoneOffsetsNum - 1;
    }
    else
    {
        Octave = (Degree - 1) / ScaleSemitoneOffsetsNum;
        Degree = (Degree - 1) % ScaleSemitoneOffsetsNum;
    }
    return Root + Octave * 12 + SourceExpressions[2 + Degree]->ComputeValue(Context).Int;
}

#if WITH_EDITOR
FString FConcordToNoteExpression::ToString() const
{
    return FString::Printf(TEXT(R"Expression(
        const int32 Root = $int0;
        int32 Degree = $int1;
        const int32 ScaleSemitoneOffsetsNum = %i;
        int32 Octave;
        if (Degree <= 0)
        {
            Octave = Degree / ScaleSemitoneOffsetsNum - 1;
            Degree = ScaleSemitoneOffsetsNum + Degree %% ScaleSemitoneOffsetsNum - 1;
        }
        else
        {
            Octave = (Degree - 1) / ScaleSemitoneOffsetsNum;
            Degree = (Degree - 1) %% ScaleSemitoneOffsetsNum;
        }
        return Root + Octave * 12 + $int2array(Degree);
    )Expression"), SourceExpressions.Num() - 2);
}
#endif

FString UConcordTransformerScale::GetCategory() const
{
    return TEXT("Music");
}

TArray<FConcordSharedExpression> UConcordTransformerScale::GetSourceExpressions(const FConcordMultiIndex& InMultiIndex) const
{
    TArray<FConcordSharedExpression> SourceExpressions;
    const int32 NumScaleSemitoneOffsets = GetConnectedTransformers().Last()->GetShape().Last();
    SourceExpressions.Reserve(GetConnectedTransformers().Num() - 1 + NumScaleSemitoneOffsets);
    for (int32 Index = 0; Index < GetConnectedTransformers().Num() - 1; ++Index)
    {
        const UConcordTransformer* ConnectedTransformer = GetConnectedTransformers()[Index];
        FConcordMultiIndex ConnectedMultiIndex = InMultiIndex;
        ConcordShape::Unbroadcast(ConnectedMultiIndex, ConnectedTransformer->GetShape());
        SourceExpressions.Add(ConnectedTransformer->GetExpression(ConnectedMultiIndex));
    }
    const UConcordTransformer* ConnectedTransformer = GetConnectedTransformers().Last();
    FConcordShape ConnectedShape = ConnectedTransformer->GetShape();
    ConnectedShape.RemoveAt(ConnectedShape.Num() - 1, 1, false);
    FConcordMultiIndex ConnectedMultiIndex = InMultiIndex;
    ConcordShape::Unbroadcast(ConnectedMultiIndex, ConnectedShape);
    for (ConnectedMultiIndex.Add(0); ConnectedMultiIndex.Last() < NumScaleSemitoneOffsets; ++ConnectedMultiIndex.Last())
        SourceExpressions.Add(ConnectedTransformer->GetExpression(ConnectedMultiIndex));
    return MoveTemp(SourceExpressions);
}

UConcordVertex::FSetupResult UConcordTransformerScale::Setup(TOptional<FString>& OutErrorMessage)
{
    TArray<FConcordShape> ConnectedShapes = GetConnectedShapes();
    const int32 NumSemitoneOffsets = ConnectedShapes.Last().Pop(false);
    if (NumSemitoneOffsets > 32) { OutErrorMessage = TEXT("The number of semitones offsets that can be in a scale is limited to 32."); return {}; }
    TOptional<FConcordShape> BroadcastedShape = ConcordShape::Broadcast(ConnectedShapes);
    if (!BroadcastedShape) { OutErrorMessage = TEXT("Input shapes could not be broadcasted together."); return {}; }
    return { BroadcastedShape.GetValue(), EConcordValueType::Int };
}

FText UConcordTransformerToDegree::GetDisplayName() const
{
    return INVTEXT("To Degree");
}

FText UConcordTransformerToDegree::GetTooltip() const
{
    return INVTEXT("Computes the scale degree of the input note.");
}

TArray<UConcordVertex::FInputInfo> UConcordTransformerToDegree::GetInputInfo() const
{
    return
    {
        { {}, EConcordValueType::Int, INVTEXT("Root"), "60"},
        { {}, EConcordValueType::Int, INVTEXT("Note"), "60"},
        { {}, EConcordValueType::Int, INVTEXT("Default"), "1"},
        { {}, EConcordValueType::Int, INVTEXT("Scale"), "0"}
    };
}

FConcordSharedExpression UConcordTransformerToDegree::GetExpression(const FConcordMultiIndex& InMultiIndex) const
{
    return MakeShared<const FConcordToDegreeExpression>(GetSourceExpressions(InMultiIndex));
}

FText UConcordTransformerToNote::GetDisplayName() const
{
    return INVTEXT("To Note");
}

FText UConcordTransformerToNote::GetTooltip() const
{
    return INVTEXT("Computes a note number from the input scale degree.");
}

TArray<UConcordVertex::FInputInfo> UConcordTransformerToNote::GetInputInfo() const
{
    return
    {
        { {}, EConcordValueType::Int, INVTEXT("Root"), "60"},
        { {}, EConcordValueType::Int, INVTEXT("Degree"), "1"},
        { {}, EConcordValueType::Int, INVTEXT("Scale"), "0"}
    };
}

FConcordSharedExpression UConcordTransformerToNote::GetExpression(const FConcordMultiIndex& InMultiIndex) const
{
    return MakeShared<const FConcordToNoteExpression>(GetSourceExpressions(InMultiIndex));
}
