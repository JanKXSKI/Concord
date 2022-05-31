// Copyright Jan Klimaschewski. All Rights Reserved.

#include "Transformers/ConcordTransformerClamp.h"
#include "Transformers/ConcordTransformerFlank.h"
#include "Transformers/ConcordTransformerFlankDistance.h"
#include "Transformers/ConcordTransformerGetDynamic.h"
#include "Transformers/ConcordTransformerMap.h"
#include "Transformers/ConcordTransformerMapDyn.h"
#include "Transformers/ConcordTransformerPositionInOctave.h"
#include "Transformers/ConcordTransformerTable.h"
#include "Transformers/ConcordTransformerInterpolate.h"
#include "Transformers/ConcordTransformerChordRoot.h"
#include "Transformers/ConcordTransformerSetDynamic.h"
#include <type_traits>

#if WITH_EDITOR
using namespace Concord;
#endif

template<typename FValue>
FConcordValue FConcordClampExpression<FValue>::ComputeValue(const FConcordExpressionContext<float>& Context) const
{
    const FValue X = SourceExpressions[0]->ComputeValue(Context).Get<FValue>();
    const FValue Min = SourceExpressions[1]->ComputeValue(Context).Get<FValue>();
    const FValue Max = SourceExpressions[2]->ComputeValue(Context).Get<FValue>();
    return X < Min ? Min : (X > Max ? Max : X);
}

#if WITH_EDITOR
template<typename FValue>
FString FConcordClampExpression<FValue>::ToString() const
{
    return FString::Format(TEXT(R"Expression(
        const {0} X = {1}0;
        const {0} Min = {1}1;
        const {0} Max = {1}2;
        return X < Min ? Min : (X > Max ? Max : X);
)Expression"), { NativeTypeString<FValue>(), ExpressionVariableString<FValue>() });
}
#endif

template class FConcordClampExpression<int32>;
template class FConcordClampExpression<float>;

template<typename FValue>
FConcordValue FConcordFlankExpression<FValue>::ComputeValue(const FConcordExpressionContext<float>& Context) const
{
    const FValue Previous = SourceExpressions[0]->ComputeValue(Context).Get<FValue>();
    const FValue Current = SourceExpressions[1]->ComputeValue(Context).Get<FValue>();
    return int32(Previous != Current);
}

#if WITH_EDITOR
template<typename FValue>
FString FConcordFlankExpression<FValue>::ToString() const
{
    return FString::Format(TEXT(R"Expression(
        const {0} Previous = {1}0;
        const {0} Current = {1}1;
        return int32(Previous != Current);
)Expression"), { NativeTypeString<FValue>(), ExpressionVariableString<FValue>() });
}
#endif

template class FConcordFlankExpression<int32>;
template class FConcordFlankExpression<float>;

template<typename FValue>
FConcordValue FConcordFlankDistanceExpression<FValue>::ComputeValue(const FConcordExpressionContext<float>& Context) const
{
    const FValue Value = SourceExpressions[0]->ComputeValue(Context).Get<FValue>();
    for (int32 Index = 1; Index < SourceExpressions.Num(); ++Index)
        if (SourceExpressions[Index]->ComputeValue(Context).Get<FValue>() != Value)
            return Index;
    return SourceExpressions.Num();
}

#if WITH_EDITOR
template<typename FValue>
FString FConcordFlankDistanceExpression<FValue>::ToString() const
{
    return FString::Format(TEXT(R"Expression(
        const {0} Value = {1}0;
        for (int32 Index = 1; Index < {2}; ++Index)
            if ({1}1array(Index - 1) != Value)
                return Index;
        return {2};
)Expression"), { NativeTypeString<FValue>(), ExpressionVariableString<FValue>(), SourceExpressions.Num() });
}
#endif

template class FConcordFlankDistanceExpression<int32>;
template class FConcordFlankDistanceExpression<float>;

FConcordValue FConcordGetExpression::ComputeValue(const FConcordExpressionContext<float>& Context) const
{
    const int32 Index = SourceExpressions[0]->ComputeValue(Context).Int;
    if (Index >= 0 && Index < SourceExpressions.Num() - 2)
        return SourceExpressions[2 + Index]->ComputeValue(Context);
    return SourceExpressions[1]->ComputeValue(Context);
}

#if WITH_EDITOR
FString FConcordGetExpression::ToString() const
{
    return FString::Format(TEXT(R"Expression(
        const int32 Index = $int0;
        if (Index >= 0 && Index < {0})
            return $any2array(Index);
        return $any1;
)Expression"), { SourceExpressions.Num() - 2 });
}
#endif

template<typename FValue>
FConcordValue FConcordMapExpression<FValue>::ComputeValue(const FConcordExpressionContext<float>& Context) const
{
    const int32 Key = SourceExpressions[0]->ComputeValue(Context).Int;
    if (const FValue* Value = Map->Find(Key)) return *Value;
    return DefaultValue;
}

#if WITH_EDITOR
template<typename FValue>
FString FConcordMapExpression<FValue>::ToString() const
{
    FString String = TEXT(R"Expression(
        const int32 Key = $int0;
        switch(Key)
        {
)Expression");
    for (const TPair<int32, FValue>& Pair : Map.Get())
        String += FString::Printf(TEXT("        case %i: return %s;\n"), Pair.Key, *LexToString(Pair.Value));
    String += FString::Printf(TEXT("        default: return %s;\n        }\n"), *LexToString(DefaultValue));
    return MoveTemp(String);
}
#endif

template class FConcordMapExpression<int32>;
template class FConcordMapExpression<float>;

FConcordValue FConcordMapDynExpression::ComputeValue(const FConcordExpressionContext<float>& Context) const
{
    const int32 Key = SourceExpressions[0]->ComputeValue(Context).Int;
    for (int32 PairIndex = 0; PairIndex < NumPairs; ++PairIndex)
    {
        const int32 CandidateKey = SourceExpressions[2 + PairIndex]->ComputeValue(Context).Int;
        if (Key == CandidateKey) return SourceExpressions[2 + NumPairs + PairIndex]->ComputeValue(Context);
    }
    return SourceExpressions[1]->ComputeValue(Context);
}

#if WITH_EDITOR
FString FConcordMapDynExpression::ToString() const
{
    return FString::Format(TEXT(R"Expression(
        const int32 Key = $int0;
        for (int32 PairIndex = 0; PairIndex < {0}; ++PairIndex)
        {
            const int32 CandidateKey = $int2array{0}(PairIndex);
            if (Key == CandidateKey) return $any{1}array(PairIndex);
        }
        return $any1;
)Expression"), { NumPairs, 2 + NumPairs });
}
#endif

FConcordValue FConcordPositionInOctaveExpression::ComputeValue(const FConcordExpressionContext<float>& Context) const
{
    const int32 Root = SourceExpressions[0]->ComputeValue(Context).Int - 11 * 12; // ensure Root < Note for sensible notes
    const int32 Note = SourceExpressions[1]->ComputeValue(Context).Int;
    return (Note - Root) % 12;
}

#if WITH_EDITOR
FString FConcordPositionInOctaveExpression::ToString() const
{
    return TEXT(R"Expression(
        const int32 Root = $int0 - 11 * 12;
        const int32 Note = $int1;
        return (Note - Root) % 12;
)Expression");
}
#endif

FConcordValue FConcordTransformerTableExpression::ComputeValue(const FConcordExpressionContext<float>& Context) const
{
    int32 FlatIndex = 0; int32 Stride = 1;
    for (int32 DimIndex = TableShape.Num() - 1; DimIndex >= 0; --DimIndex)
    {
        const int32 Index = SourceExpressions[DimIndex]->ComputeValue(Context).Int;
        if (Index < 0 || Index >= TableShape[DimIndex]) return 0;
        FlatIndex += Index * Stride; Stride *= TableShape[DimIndex];
    }
    return SourceExpressions[TableShape.Num() + FlatIndex]->ComputeValue(Context);
}

#if WITH_EDITOR
FString FConcordTransformerTableExpression::ToString() const
{
    FString String = TEXT("\n        int32 FlatIndex = 0, Stride = 1, Index;\n");
    for (int32 DimIndex = TableShape.Num() - 1; DimIndex >= 0; --DimIndex)
    {
        String += FString::Printf(TEXT("        Index = $int0array%i(%i);\n"), TableShape.Num(), DimIndex);
        String += FString::Printf(TEXT("        if (Index < 0 || Index >= %i) return 0;\n"), TableShape[DimIndex]);
        String += FString::Printf(TEXT("        FlatIndex += Index * Stride; Stride *= %i;\n"), TableShape[DimIndex]);
    }
    String += FString::Printf(TEXT("        return $any%iarray(FlatIndex);\n"), TableShape.Num());
    return MoveTemp(String);
}
#endif

int32 FConcordInterpolateExpression::GetIndex(int32 Offset, int32 Num, const FConcordExpressionContext<float>& Context) const
{
    const int32 NumValues = SourceExpressions.Num() / 2;
    for (int32 Index = 0; Index < Num; ++Index)
        if (SourceExpressions[NumValues + Offset + Index]->ComputeValue(Context).Int)
            return Index;
    return -1;
}

#if WITH_EDITOR
FString FConcordInterpolateExpression::GetIndexToString(const TCHAR* Suffix, int32 Offset, int32 Num) const
{
    return FString::Printf(TEXT(R"Expression(
        int32 Index%s = -1;
        for (int32 Index = 0; Index < %i; ++Index)
            if ($int%iarray(%i + Index))
            {
                Index%s = Index;
                break;
            }
)Expression"), Suffix, Num, SourceExpressions.Num() / 2, Offset, Suffix);
}
#endif

template<typename FValue>
FConcordValue FConcordLinearInterpolateExpression<FValue>::ComputeValue(const FConcordExpressionContext<float>& Context) const
{
    const int32 IndexA = GetIndex(0, ForwardCount, Context);
    const int32 IndexB = GetIndex(ForwardCount, SourceExpressions.Num() / 2 - ForwardCount, Context);
    if (IndexA < 0) return (IndexB < 0) ? SourceExpressions[0]->ComputeValue(Context) : SourceExpressions[ForwardCount + IndexB]->ComputeValue(Context);
    if (IndexB < 0) return (IndexA < 0) ? SourceExpressions[0]->ComputeValue(Context) : SourceExpressions[IndexA]->ComputeValue(Context);
    const float WeightA = (IndexB + 1) / float(IndexA + IndexB + 1);
    const float WeightB = IndexA / float(IndexA + IndexB + 1);
    if constexpr (std::is_same_v<FValue, int32>) return FMath::RoundToInt(WeightA * SourceExpressions[IndexA]->ComputeValue(Context).Int +
                                                                          WeightB * SourceExpressions[ForwardCount + IndexB]->ComputeValue(Context).Int);
    else return WeightA * SourceExpressions[IndexA]->ComputeValue(Context).Float +
                WeightB * SourceExpressions[ForwardCount + IndexB]->ComputeValue(Context).Float;
}

#if WITH_EDITOR
template<typename FValue>
FString FConcordLinearInterpolateExpression<FValue>::ToString() const
{
    const FString WeightsString = FString::Format(TEXT(R"Expression(
        if (IndexA < 0) return (IndexB < 0) ? $any0array{0}(0) : $any0array{0}({1} + IndexB);
        if (IndexB < 0) return (IndexA < 0) ? $any0array{0}(0) : $any0array{0}(IndexA);
        const float WeightA = (IndexB + 1) / float(IndexA + IndexB + 1);
        const float WeightB = IndexA / float(IndexA + IndexB + 1);
)Expression"), { SourceExpressions.Num() / 2, ForwardCount });
    FString InterpolateString;
    if constexpr (std::is_same_v<FValue, int32>) InterpolateString = FString::Format(TEXT(R"Expression(
        return FMath::RoundToInt(WeightA * $any0array{0}(IndexA) + WeightB * $any0array{0}({1} + IndexB));
)Expression"), { SourceExpressions.Num() / 2, ForwardCount });
    else InterpolateString = FString::Format(TEXT(R"Expression(
        return WeightA * $any0array{0}(IndexA) + WeightB * $any0array{0}({1} + IndexB);
)Expression"), { SourceExpressions.Num() / 2, ForwardCount });
    return GetIndexToString(TEXT("A"), 0, ForwardCount) + GetIndexToString(TEXT("B"), ForwardCount, SourceExpressions.Num() / 2 - ForwardCount) + WeightsString + InterpolateString;
}
#endif

template class FConcordLinearInterpolateExpression<int32>;
template class FConcordLinearInterpolateExpression<float>;

FConcordValue FConcordConstantInterpolateExpression::ComputeValue(const FConcordExpressionContext<float>& Context) const
{
    const int32 Index = GetIndex(0, SourceExpressions.Num() / 2, Context);
    if (Index < 0) return SourceExpressions[0]->ComputeValue(Context);
    return SourceExpressions[Index]->ComputeValue(Context);
}

#if WITH_EDITOR
FString FConcordConstantInterpolateExpression::ToString() const
{
    return GetIndexToString(TEXT("A"), 0, SourceExpressions.Num() / 2) + FString::Format(TEXT(R"Expression(
        if (IndexA < 0) return $any0array{0}(0);
        return $any0array{0}(IndexA);
)Expression"), { SourceExpressions.Num() / 2 });
}
#endif

template<bool bBackward>
FConcordValue FConcordConstantBidirectionalInterpolateExpression<bBackward>::ComputeValue(const FConcordExpressionContext<float>& Context) const
{
    if (SourceExpressions[SourceExpressions.Num() / 2]->ComputeValue(Context).Int)
        return SourceExpressions[0]->ComputeValue(Context);
    const int32 Offsets[2] = { 1, ForwardCount };
    const int32 Nums[2] = { ForwardCount - 1, SourceExpressions.Num() / 2 - ForwardCount };
    const int32 Order[2] = { int32(bBackward), 1 - int32(bBackward) };
    for (int32 Dir = 0; Dir < 2; ++Dir)
    {
        const int32 Index = GetIndex(Offsets[Order[Dir]], Nums[Order[Dir]], Context);
        if (Index >= 0) return SourceExpressions[Offsets[Order[Dir]] + Index]->ComputeValue(Context);
    }
    return SourceExpressions[0]->ComputeValue(Context);
}

#if WITH_EDITOR
template<bool bBackward>
FString FConcordConstantBidirectionalInterpolateExpression<bBackward>::ToString() const
{
    FString Ret; Ret.Reserve(256);
    Ret += FString::Format(TEXT(R"Expression(
        if ($int{0}array(0)) return $any0array{0}(0);
)Expression"), { SourceExpressions.Num() / 2 });
    const int32 Offsets[2] = { 0, ForwardCount };
    const int32 Nums[2] = { ForwardCount, SourceExpressions.Num() / 2 - ForwardCount };
    const int32 Order[2] = { int32(bBackward), 1 - int32(bBackward) };
    const TCHAR* Suffixes[2] = { TEXT("A"), TEXT("B") };
    for (int32 Dir = 0; Dir < 2; ++Dir)
    {
        Ret += GetIndexToString(Suffixes[Dir], Offsets[Order[Dir]], Nums[Order[Dir]]);
        Ret += FString::Format(TEXT(R"Expression(
        if (Index{0} >= 0) return $any0array{1}({2} + Index{0});
)Expression"), { Suffixes[Dir], SourceExpressions.Num() / 2, Offsets[Order[Dir]] });
    }
    Ret += FString::Format(TEXT(R"Expression(
        return $any0array{0}(0);
)Expression"), { SourceExpressions.Num() / 2 });
    return MoveTemp(Ret);
}
#endif

template class FConcordConstantBidirectionalInterpolateExpression<false>;
template class FConcordConstantBidirectionalInterpolateExpression<true>;

FConcordValue FConcordChordRootExpression::ComputeValue(const FConcordExpressionContext<float>& Context) const
{
    int32 LowestNote = 128;
    int32 SecondToLowestNote = 128;
    for (const auto& SourceExpression : SourceExpressions)
    {
        const int32 Note = SourceExpression->ComputeValue(Context).Int;
        if (Note < LowestNote) { SecondToLowestNote = LowestNote; LowestNote = Note; }
        else if (Note < SecondToLowestNote) SecondToLowestNote = Note;
    }
    if (SecondToLowestNote - LowestNote == 5) return SecondToLowestNote;
    return LowestNote;
}

#if WITH_EDITOR
FString FConcordChordRootExpression::ToString() const
{
    return FString::Format(TEXT(R"Expression(
        int32 LowestNote = 128;
        int32 SecondToLowestNote = 128;
        for (int32 Index = 0; Index < {0}; ++Index)
        {
            const int32 Note = $int0array(Index);
            if (Note < LowestNote) { SecondToLowestNote = LowestNote; LowestNote = Note; }
            else if (Note < SecondToLowestNote) SecondToLowestNote = Note;
        }
        if (SecondToLowestNote - LowestNote == 5) return SecondToLowestNote;
        return LowestNote;
)Expression"), { SourceExpressions.Num() });
}
#endif

FConcordValue FConcordSetExpression::ComputeValue(const FConcordExpressionContext<float>& Context) const
{
    for (int32 SourceExpressionIndex = 1; SourceExpressionIndex < SourceExpressions.Num(); SourceExpressionIndex += 2)
    {
        const int32 TargetIndex = SourceExpressions[SourceExpressionIndex]->ComputeValue(Context).Int;
        if (TargetIndex == Index) return SourceExpressions[SourceExpressionIndex + 1]->ComputeValue(Context);
    }
    return SourceExpressions[0]->ComputeValue(Context);
}

#if WITH_EDITOR
FString FConcordSetExpression::ToString() const
{
    FString Ret;
    for (int32 SourceExpressionIndex = 1; SourceExpressionIndex < SourceExpressions.Num(); SourceExpressionIndex += 2)
    {
        Ret += FString::Format(TEXT(R"Expression(
        if ($int{0} == {1}) return $any{2};
)Expression"), { SourceExpressionIndex, Index, SourceExpressionIndex + 1 });
    }
    Ret += TEXT(R"Expression(
        return $any0;
)Expression");
    return MoveTemp(Ret);
}
#endif
