// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerPR.generated.h"

struct FConcordNote
{
    int32 Degree;
    int32 Onset;
    int32 Length;
};

template<typename FConcordNoteVisitor>
struct FConcordNoteVisitorBase
{
    const TArrayView<const FConcordSharedExpression> Expressions;
    int32 Index = 0;

    [[nodiscard]] bool operator()(const FConcordExpressionContext<float>& Context, bool bBackward = false)
    {
        FConcordNote Note;
        while (true)
        {
            TOptional<int32> OptionalDegree = SeekDegreeEvent(Context, bBackward);
            if (!OptionalDegree) return false;
            Note.Degree = OptionalDegree.GetValue();
            Note.Onset = Index;
            ++Index;
            SeekForward(Context, [](int32 NoteValue){ return NoteValue != 0; });
            Note.Length = Index - Note.Onset;
            if (bBackward) Index = Note.Onset - 1;
            if (static_cast<FConcordNoteVisitor*>(this)->Visit(Note)) return true;
        }
        checkNoEntry();
        return false;
    }
private:
    template<typename FFilter>
    TOptional<int32> SeekForward(const FConcordExpressionContext<float>& Context, const FFilter& Filter)
    {
        for (; Index < Expressions.Num(); ++Index)
            if (int32 NoteValue = Expressions[Index]->ComputeValue(Context).Int; Filter(NoteValue)) return NoteValue;
        return {};
    }

    template<typename FFilter>
    TOptional<int32> SeekBackward(const FConcordExpressionContext<float>& Context, const FFilter& Filter)
    {
        for (; Index >= 0; --Index)
            if (int32 NoteValue = Expressions[Index]->ComputeValue(Context).Int; Filter(NoteValue)) return NoteValue;
        return {};
    }

    TOptional<int32> SeekDegreeEvent(const FConcordExpressionContext<float>& Context, bool bBackward)
    {
        const auto Filter = [](int32 NoteValue){ return NoteValue > 0; };
        if (bBackward) return SeekBackward(Context, Filter);
        else return SeekForward(Context, Filter);
    }
};

template<typename FImpl>
class FConcordPRExpression : public FConcordComputingExpression
{
public:
    using FConcordComputingExpression::FConcordComputingExpression;
#if WITH_EDITOR
    FString ToString() const override { return TEXT("PR ToString() not implemented."); }
#endif
    FConcordValue ComputeValue(const FConcordExpressionContext<float>& Context) const override
    {
        return SourceExpressions[0]->ComputeValue(Context).Float * static_cast<const FImpl*>(this)->ComputeScoreClass(Context);
    }
protected:
    TArrayView<const FConcordSharedExpression> GetDegreeExpressions() const
    {
        return TArrayView<const FConcordSharedExpression>(SourceExpressions.GetData() + GetDegreesOffset(), GetNumDegrees());
    }
private:
    int32 GetDegreesOffset() const { return 1 + FImpl::NumAdditionalSourceExpressions; }
    int32 GetNumDegrees() const { return SourceExpressions.Num() - GetDegreesOffset(); }
};

template<typename FImpl>
class FConcordPR2GroupsExpression : public FConcordPRExpression<FConcordPR2GroupsExpression<FImpl>>
{
public:
    using FBase = FConcordPRExpression<FConcordPR2GroupsExpression<FImpl>>;
    using FBase::FBase;
    static inline int NumAdditionalSourceExpressions = 1;
    int32 ComputeScoreClass(const FConcordExpressionContext<float>& Context) const
    {
        return static_cast<const FImpl*>(this)->ComputeScoreClass(Context);
    }
private:
    struct FNoteVisitor : FConcordNoteVisitorBase<FNoteVisitor>
    {
        bool Visit(const FConcordNote& InNote)
        {
            Note = InNote;
            return true;
        }
        FConcordNote Note;
    };
protected:
    int32 GetBoundaryIndex(const FConcordExpressionContext<float>& Context) const
    {
        return FBase::SourceExpressions[1]->ComputeValue(Context).Int;
    }

    [[nodiscard]] bool GetNotes(const FConcordExpressionContext<float>& Context, FConcordNote OutNotes[4]) const
    {
        const TArrayView<const FConcordSharedExpression> DegreeExpressions { GetDegreeExpressions() };
        int32 BoundaryIndex = GetBoundaryIndex(Context);
        if (BoundaryIndex < 0 || BoundaryIndex >= DegreeExpressions.Num()) return false;
        if (DegreeExpressions[BoundaryIndex]->ComputeValue(Context).Int <= 0) return false;
        FNoteVisitor NoteVisitor { DegreeExpressions, BoundaryIndex };
        if (!NoteVisitor(Context, true)) return false;
        OutNotes[2] = NoteVisitor.Note;
        if (!NoteVisitor(Context, true)) return false;
        OutNotes[1] = NoteVisitor.Note;
        if (!NoteVisitor(Context, true)) return false;
        OutNotes[0] = NoteVisitor.Note;
        NoteVisitor.Index = BoundaryIndex + OutNotes[2].Length;
        if (!NoteVisitor(Context)) return false;
        OutNotes[3] = NoteVisitor.Note;
        return true;
    }
};

class FConcordGPR1Expression : public FConcordPRExpression<FConcordGPR1Expression>
{
public:
    using FBase = FConcordPRExpression<FConcordGPR1Expression>;
    using FBase::FBase;
    static inline int NumAdditionalSourceExpressions = 0;

    struct FNoteVisitor : FConcordNoteVisitorBase<FNoteVisitor>
    {
        bool Visit(const FConcordNote& Note)
        {
            ++NumNotes;
            return NumNotes > 2;
        }
        int32 NumNotes { 0 };
    };

    int32 ComputeScoreClass(const FConcordExpressionContext<float>& Context) const
    {
        FNoteVisitor NoteVisitor { GetDegreeExpressions() };
        if (NoteVisitor(Context)) return 0;
        switch (NoteVisitor.NumNotes)
        {
        case 0: return -4;
        case 1: return -2;
        default: return -1;
        }
    }
};

class FConcordGPR2aExpression : public FConcordPR2GroupsExpression<FConcordGPR2aExpression>
{
public:
    using FBase = FConcordPR2GroupsExpression<FConcordGPR2aExpression>;
    using FBase::FBase;
    int32 ComputeScoreClass(const FConcordExpressionContext<float>& Context) const
    {
        FConcordNote Notes[4];
        if (!GetNotes(Context, Notes)) return 0;
        int32 Intervals[]
        {
            Notes[1].Onset - (Notes[0].Onset + Notes[0].Length),
            Notes[2].Onset - (Notes[1].Onset + Notes[1].Length),
            Notes[3].Onset - (Notes[2].Onset + Notes[2].Length)
        };
        if (Intervals[1] <= Intervals[0] || Intervals[1] <= Intervals[2]) return 0;
        return Intervals[1] - Intervals[0] + Intervals[1] - Intervals[2];
    }
};

class FConcordGPR2bExpression : public FConcordPR2GroupsExpression<FConcordGPR2bExpression>
{
public:
    using FBase = FConcordPR2GroupsExpression<FConcordGPR2bExpression>;
    using FBase::FBase;
    int32 ComputeScoreClass(const FConcordExpressionContext<float>& Context) const
    {
        FConcordNote Notes[4];
        if (!GetNotes(Context, Notes)) return 0;
        int32 Intervals[]
        {
            Notes[1].Onset - Notes[0].Onset,
            Notes[2].Onset - Notes[1].Onset,
            Notes[3].Onset - Notes[2].Onset
        };
        if (Intervals[1] <= Intervals[0] || Intervals[1] <= Intervals[2]) return 0;
        return Intervals[1] - Intervals[0] + Intervals[1] - Intervals[2];
    }
};

UCLASS(Abstract)
class CONCORD_API UConcordTransformerPR : public UConcordTransformer
{
    GENERATED_BODY()
public:    
    TArray<FInputInfo> GetInputInfo() const override
    {
        TArray<FInputInfo> InputInfo;
        InputInfo.Add({ {}, EConcordValueType::Int, INVTEXT("Degrees"), "0" });
        InputInfo.Add({ {}, EConcordValueType::Float, INVTEXT("Scale"), "1.0" });
        AddAdditionalInputs(InputInfo);
        return InputInfo;
    }

    FString GetCategory() const override { return TEXT("GTTM"); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& InMultiIndex) const override
    {
        TArray<FConcordSharedExpression> SourceExpressions;
        AddBroadcastedConnectedExpressions(SourceExpressions, 1, GetConnectedTransformers().Num() - 1, InMultiIndex);
        FConcordMultiIndex DegreesIndex { InMultiIndex };
        ConcordShape::Unbroadcast(DegreesIndex, DegreesShape);
        for (DegreesIndex.Add(0); DegreesIndex.Last() < GetConnectedTransformers()[0]->GetShape().Last(); ++DegreesIndex.Last())
            SourceExpressions.Add(GetConnectedTransformers()[0]->GetExpression(DegreesIndex));
        return MakeExpression(MoveTemp(SourceExpressions));
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        TArray<FConcordShape> ConnectedShapes = GetConnectedShapes();
        ConnectedShapes[0].Pop(false);
        DegreesShape = ConnectedShapes[0];
        TOptional<FConcordShape> BroadcastedShape = ConcordShape::Broadcast(ConnectedShapes);
        if (!BroadcastedShape) { OutErrorMessage = TEXT("Input shapes could not be broadcasted together."); return {}; }
        return { BroadcastedShape.GetValue(), EConcordValueType::Float };
    }

    FConcordShape DegreesShape;

    virtual void AddAdditionalInputs(TArray<FInputInfo>& InOutInputInfo) const {}

    virtual FConcordSharedExpression MakeExpression(TArray<FConcordSharedExpression>&& SourceExpressions) const
    {
        checkNoEntry();
        return MakeShared<const FConcordValueExpression<int32>>(0);
    }
};

UCLASS()
class CONCORD_API UConcordTransformerGPR1 : public UConcordTransformerPR
{
    GENERATED_BODY()
public:
    FText GetDisplayName() const override { return INVTEXT("GPR1"); }
    FText GetTooltip() const override { return INVTEXT("Tries to apply grouping preference rule 1 to the input degrees."); }
private:
    FConcordSharedExpression MakeExpression(TArray<FConcordSharedExpression>&& SourceExpressions) const override
    {
        return MakeShared<const FConcordGPR1Expression>(MoveTemp(SourceExpressions));
    }
};

UCLASS(Abstract)
class CONCORD_API UConcordTransformerPR2Groups : public UConcordTransformerPR
{
    GENERATED_BODY()
private:
    void AddAdditionalInputs(TArray<FInputInfo>& InOutInputInfo) const override
    {
        InOutInputInfo.Add({ {}, EConcordValueType::Int, INVTEXT("Boundary Index"), "0" });
    }
};

UCLASS()
class CONCORD_API UConcordTransformerGPR2a : public UConcordTransformerPR2Groups
{
    GENERATED_BODY()
public:
    FText GetDisplayName() const override { return INVTEXT("GPR2a"); }
    FText GetTooltip() const override { return INVTEXT("Tries to apply grouping preference rule 2a before the boundary note."); }
private:
    FConcordSharedExpression MakeExpression(TArray<FConcordSharedExpression>&& SourceExpressions) const override
    {
        return MakeShared<const FConcordGPR2aExpression>(MoveTemp(SourceExpressions));
    }
};


UCLASS()
class CONCORD_API UConcordTransformerGPR2b : public UConcordTransformerPR2Groups
{
    GENERATED_BODY()
public:
    FText GetDisplayName() const override { return INVTEXT("GPR2b"); }
    FText GetTooltip() const override { return INVTEXT("Tries to apply grouping preference rule 2b before the boundary note."); }
private:
    FConcordSharedExpression MakeExpression(TArray<FConcordSharedExpression>&& SourceExpressions) const override
    {
        return MakeShared<const FConcordGPR2bExpression>(MoveTemp(SourceExpressions));
    }
};
