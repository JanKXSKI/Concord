// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include <array>
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
    static constexpr int NumAdditionalSourceExpressions = 0;
#if WITH_EDITOR
    FString ToString() const override { return TEXT("PR ToString() not implemented."); }
#endif
    FConcordValue ComputeValue(const FConcordExpressionContext<float>& Context) const override
    {
        return int32(static_cast<const FImpl*>(this)->DoesPreferenceRuleApply(Context));
    }
protected:
    TArrayView<const FConcordSharedExpression> GetDegreeExpressions() const
    {
        return TArrayView<const FConcordSharedExpression>(SourceExpressions.GetData() + GetDegreesOffset(), GetNumDegrees());
    }
    int32 GetDegreesOffset() const { return FImpl::NumAdditionalSourceExpressions; }
    int32 GetNumDegrees() const { return SourceExpressions.Num() - GetDegreesOffset(); }
};

template<typename FImpl>
class FConcordPR2GroupsExpression : public FConcordPRExpression<FImpl>
{
public:
    using FBase = FConcordPRExpression<FImpl>;
    using FBase::FBase;
    static constexpr int NumAdditionalSourceExpressions = FBase::NumAdditionalSourceExpressions + 1;
protected:
    int32 GetBoundaryIndex(const FConcordExpressionContext<float>& Context) const
    {
        return FMath::Clamp(FBase::SourceExpressions[FBase::NumAdditionalSourceExpressions]->ComputeValue(Context).Int, 0, FBase::GetNumDegrees() - 1);
    }
};

template<typename FImpl>
class FConcordPRAdjacentGroupsExpression : public FConcordPR2GroupsExpression<FImpl>
{
public:
    using FBase = FConcordPR2GroupsExpression<FImpl>;
    using FBase::FBase;
    static constexpr int NumAdditionalSourceExpressions = FBase::NumAdditionalSourceExpressions;
private:
    struct FNoteVisitor : FConcordNoteVisitorBase<FNoteVisitor>
    {
        bool Visit(const FConcordNote& InNote)
        {
            OutNotes[OutNoteIndex] = InNote;
            return true;
        }
        std::array<FConcordNote, 4>& OutNotes;
        int32 OutNoteIndex { 2 };
    };
protected:
    [[nodiscard]] bool GetNotes(const FConcordExpressionContext<float>& Context, std::array<FConcordNote, 4>& OutNotes) const
    {
        const TArrayView<const FConcordSharedExpression> DegreeExpressions { FBase::GetDegreeExpressions() };
        int32 BoundaryIndex = FBase::GetBoundaryIndex(Context);
        if (DegreeExpressions[BoundaryIndex]->ComputeValue(Context).Int <= 0) return false;
        FNoteVisitor NoteVisitor { { DegreeExpressions, BoundaryIndex }, OutNotes, 2 };
        if (!NoteVisitor(Context, true)) return false;
        NoteVisitor.OutNoteIndex = 1;
        if (!NoteVisitor(Context, true)) return false;
        NoteVisitor.OutNoteIndex = 0;
        if (!NoteVisitor(Context, true)) return false;
        NoteVisitor.Index = BoundaryIndex + OutNotes[2].Length;
        NoteVisitor.OutNoteIndex = 3;
        if (!NoteVisitor(Context)) return false;
        return true;
    }
};

template<typename FImpl>
class FConcordPRAdjacentGroupsIntervalExpression : public FConcordPRAdjacentGroupsExpression<FImpl>
{
public:
    using FBase = FConcordPRAdjacentGroupsExpression<FImpl>;
    using FBase::FBase;
    static constexpr int NumAdditionalSourceExpressions = FBase::NumAdditionalSourceExpressions + 3;
    bool DoesPreferenceRuleApply(const FConcordExpressionContext<float>& Context) const
    {
        std::array<FConcordNote, 4> Notes;
        if (!FBase::GetNotes(Context, Notes)) return false;
        for (int32 IntervalIndex = 0; IntervalIndex < 3; ++IntervalIndex)
        {
            const int32 TargetInterval = FBase::SourceExpressions[FBase::NumAdditionalSourceExpressions + IntervalIndex]->ComputeValue(Context).Int;
            const int32 FoundInterval = FImpl::GetInterval(Notes[IntervalIndex], Notes[IntervalIndex + 1]);
            if (FoundInterval != TargetInterval) return false;
        }
        return true;
    }
};

class FConcordGPR1Expression : public FConcordPRExpression<FConcordGPR1Expression>
{
public:
    using FBase = FConcordPRExpression<FConcordGPR1Expression>;
    using FBase::FBase;
    static constexpr int NumAdditionalSourceExpressions = FBase::NumAdditionalSourceExpressions + 1;
private:
    struct FNoteVisitor : FConcordNoteVisitorBase<FNoteVisitor>
    {
        bool Visit(const FConcordNote& Note)
        {
            ++NumNotes;
            return NumNotes >= MinNumNotes;
        }
        int32 MinNumNotes { 2 };
        int32 NumNotes { 0 };
    };
public:
    bool DoesPreferenceRuleApply(const FConcordExpressionContext<float>& Context) const
    {
        const int32 MinNumNotes = SourceExpressions[FBase::NumAdditionalSourceExpressions]->ComputeValue(Context).Int;
        FNoteVisitor NoteVisitor { { GetDegreeExpressions(), 0 }, MinNumNotes };
        if (NoteVisitor(Context)) return true;
        return false;
    }
};

class FConcordGPR2aExpression : public FConcordPRAdjacentGroupsIntervalExpression<FConcordGPR2aExpression>
{
public:
    using FBase = FConcordPRAdjacentGroupsIntervalExpression<FConcordGPR2aExpression>;
    using FBase::FBase;
    static int32 GetInterval(const FConcordNote& N1, const FConcordNote& N2)
    {
        return N2.Onset - (N1.Onset + N1.Length);
    }
};

class FConcordGPR2bExpression : public FConcordPRAdjacentGroupsIntervalExpression<FConcordGPR2bExpression>
{
public:
    using FBase = FConcordPRAdjacentGroupsIntervalExpression<FConcordGPR2bExpression>;
    using FBase::FBase;
    static int32 GetInterval(const FConcordNote& N1, const FConcordNote& N2)
    {
        return N2.Onset - N1.Onset;
    }
};

class FConcordGPR3aExpression : public FConcordPRAdjacentGroupsIntervalExpression<FConcordGPR3aExpression>
{
public:
    using FBase = FConcordPRAdjacentGroupsIntervalExpression<FConcordGPR3aExpression>;
    using FBase::FBase;
    static int32 GetInterval(const FConcordNote& N1, const FConcordNote& N2)
    {
        return FMath::Abs(N2.Degree - N1.Degree);
    }
};

class FConcordGPR3dExpression : public FConcordPRAdjacentGroupsExpression<FConcordGPR3dExpression>
{
public:
    using FBase = FConcordPRAdjacentGroupsExpression<FConcordGPR3dExpression>;
    using FBase::FBase;
    static constexpr int NumAdditionalSourceExpressions = FBase::NumAdditionalSourceExpressions + 1;
    bool DoesPreferenceRuleApply(const FConcordExpressionContext<float>& Context) const
    {
        std::array<FConcordNote, 4> Notes;
        if (!GetNotes(Context, Notes)) return false;
        if (Notes[0].Length != Notes[1].Length || Notes[2].Length != Notes[3].Length) return false;
        const int32 TargetLengthDifference = SourceExpressions[FBase::NumAdditionalSourceExpressions]->ComputeValue(Context).Int;
        return (Notes[2].Length - Notes[1].Length) == TargetLengthDifference;
    }
};

class FConcordGPR5Expression : public FConcordPRAdjacentGroupsExpression<FConcordGPR5Expression>
{
public:
    using FBase = FConcordPRAdjacentGroupsExpression<FConcordGPR5Expression>;
    using FBase::FBase;
    static constexpr int NumAdditionalSourceExpressions = FBase::NumAdditionalSourceExpressions + 1;
    bool DoesPreferenceRuleApply(const FConcordExpressionContext<float>& Context) const
    {
        const int32 GroupALength = GetBoundaryIndex(Context);
        const int32 GroupBLength = GetNumDegrees() - GroupALength;
        const int32 LengthDifferenceTolerance = SourceExpressions[FBase::NumAdditionalSourceExpressions]->ComputeValue(Context).Int;
        return FMath::Abs(GroupALength - GroupBLength) <= LengthDifferenceTolerance;
    }
};

class FConcordGPR6Expression : public FConcordPR2GroupsExpression<FConcordGPR6Expression>
{
public:
    using FBase = FConcordPR2GroupsExpression<FConcordGPR6Expression>;
    using FBase::FBase;
    static constexpr int NumAdditionalSourceExpressions = FBase::NumAdditionalSourceExpressions + 1;
public:
    bool DoesPreferenceRuleApply(const FConcordExpressionContext<float>& Context) const
    {
        const int32 BoundaryIndex = GetBoundaryIndex(Context);
        const int32 GroupALength = BoundaryIndex;
        const int32 GroupBLength = GetNumDegrees() - GroupALength;
        const TArrayView<const FConcordSharedExpression> DegreeExpressions { GetDegreeExpressions() };
        const int32 MinParallelSequenceLength = SourceExpressions[FBase::NumAdditionalSourceExpressions]->ComputeValue(Context).Int;
        int32 NumParallelPitchEvents = 0;
        bool bHaveDegrees = false;
        for (int32 Index = 0; Index < FMath::Min(GroupALength, GroupBLength); ++Index)
        {
            const int32 GroupAValue = DegreeExpressions[Index]->ComputeValue(Context).Int;
            const int32 GroupBValue = DegreeExpressions[BoundaryIndex + Index]->ComputeValue(Context).Int;

            if (GroupAValue > 0 && GroupBValue > 0)
            {
                if (bHaveDegrees) ++NumParallelPitchEvents;
                bHaveDegrees = true;
            }
            else if (bHaveDegrees && GroupAValue != GroupBValue)
            {
                NumParallelPitchEvents = 0;
                bHaveDegrees = false;
            }
            else if (GroupAValue == -1 && GroupBValue == -1)
            {
                if (bHaveDegrees) ++NumParallelPitchEvents;
                bHaveDegrees = false;
            }
            if (NumParallelPitchEvents == MinParallelSequenceLength) return true;            
        }
        return false;
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
protected:
    virtual void AddAdditionalInputs(TArray<FInputInfo>& InOutInputInfo) const {}
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        TArray<FConcordShape> ConnectedShapes = GetConnectedShapes();
        ConnectedShapes[0].Pop(false);
        DegreesShape = ConnectedShapes[0];
        TOptional<FConcordShape> BroadcastedShape = ConcordShape::Broadcast(ConnectedShapes);
        if (!BroadcastedShape) { OutErrorMessage = TEXT("Input shapes could not be broadcasted together."); return {}; }
        return { BroadcastedShape.GetValue(), EConcordValueType::Int };
    }

    FConcordShape DegreesShape;

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
protected:
    void AddAdditionalInputs(TArray<FInputInfo>& InOutInputInfo) const override
    {
        InOutInputInfo.Add({ {}, EConcordValueType::Int, INVTEXT("MinNumNotes"), "2" });
    }
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
protected:
    void AddAdditionalInputs(TArray<FInputInfo>& InOutInputInfo) const override
    {
        InOutInputInfo.Add({ {}, EConcordValueType::Int, INVTEXT("BoundaryIndex"), "4" });
    }
};

UCLASS(Abstract)
class CONCORD_API UConcordTransformerPRAdjacentGroupsInterval : public UConcordTransformerPR2Groups
{
    GENERATED_BODY()
protected:
    void AddAdditionalInputs(TArray<FInputInfo>& InOutInputInfo) const override
    {
        UConcordTransformerPR2Groups::AddAdditionalInputs(InOutInputInfo);
        InOutInputInfo.Add({ {}, EConcordValueType::Int, INVTEXT("TargetInterval1"), "1" });
        InOutInputInfo.Add({ {}, EConcordValueType::Int, INVTEXT("TargetInterval2"), "2" });
        InOutInputInfo.Add({ {}, EConcordValueType::Int, INVTEXT("TargetInterval3"), "1" });
    }
};

UCLASS()
class CONCORD_API UConcordTransformerGPR2a : public UConcordTransformerPRAdjacentGroupsInterval
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
class CONCORD_API UConcordTransformerGPR2b : public UConcordTransformerPRAdjacentGroupsInterval
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

UCLASS()
class CONCORD_API UConcordTransformerGPR3a : public UConcordTransformerPRAdjacentGroupsInterval
{
    GENERATED_BODY()
public:
    FText GetDisplayName() const override { return INVTEXT("GPR3a"); }
    FText GetTooltip() const override { return INVTEXT("Tries to apply grouping preference rule 3a before the boundary note."); }
private:
    FConcordSharedExpression MakeExpression(TArray<FConcordSharedExpression>&& SourceExpressions) const override
    {
        return MakeShared<const FConcordGPR3aExpression>(MoveTemp(SourceExpressions));
    }
};

UCLASS()
class CONCORD_API UConcordTransformerGPR3d : public UConcordTransformerPR2Groups
{
    GENERATED_BODY()
public:
    FText GetDisplayName() const override { return INVTEXT("GPR3d"); }
    FText GetTooltip() const override { return INVTEXT("Tries to apply grouping preference rule 3d before the boundary note."); }
protected:
    void AddAdditionalInputs(TArray<FInputInfo>& InOutInputInfo) const override
    {
        UConcordTransformerPR2Groups::AddAdditionalInputs(InOutInputInfo);
        InOutInputInfo.Add({ {}, EConcordValueType::Int, INVTEXT("TargetLengthDifference"), "1" });
    }
private:
    FConcordSharedExpression MakeExpression(TArray<FConcordSharedExpression>&& SourceExpressions) const override
    {
        return MakeShared<const FConcordGPR3dExpression>(MoveTemp(SourceExpressions));
    }
};

UCLASS()
class CONCORD_API UConcordTransformerGPR5 : public UConcordTransformerPR2Groups
{
    GENERATED_BODY()
public:
    FText GetDisplayName() const override { return INVTEXT("GPR5"); }
    FText GetTooltip() const override { return INVTEXT("Tries to apply grouping preference rule 5 before the boundary note."); }
protected:
    void AddAdditionalInputs(TArray<FInputInfo>& InOutInputInfo) const override
    {
        UConcordTransformerPR2Groups::AddAdditionalInputs(InOutInputInfo);
        InOutInputInfo.Add({ {}, EConcordValueType::Int, INVTEXT("LengthDifferenceTolerance"), "0" });
    }
private:
    FConcordSharedExpression MakeExpression(TArray<FConcordSharedExpression>&& SourceExpressions) const override
    {
        return MakeShared<const FConcordGPR5Expression>(MoveTemp(SourceExpressions));
    }
};

UCLASS()
class CONCORD_API UConcordTransformerGPR6 : public UConcordTransformerPR2Groups
{
    GENERATED_BODY()
public:
    FText GetDisplayName() const override { return INVTEXT("GPR6"); }
    FText GetTooltip() const override { return INVTEXT("Tries to apply grouping preference rule 6 before the boundary note."); }
protected:
    void AddAdditionalInputs(TArray<FInputInfo>& InOutInputInfo) const override
    {
        UConcordTransformerPR2Groups::AddAdditionalInputs(InOutInputInfo);
        InOutInputInfo.Add({ {}, EConcordValueType::Int, INVTEXT("MinParallelSequenceLength"), "3" });
    }
private:
    FConcordSharedExpression MakeExpression(TArray<FConcordSharedExpression>&& SourceExpressions) const override
    {
        return MakeShared<const FConcordGPR6Expression>(MoveTemp(SourceExpressions));
    }
};
