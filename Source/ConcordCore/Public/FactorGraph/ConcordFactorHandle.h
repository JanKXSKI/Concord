// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordExpressionContext.h"

template<typename FFloatType>
class FConcordFactorHandleBase
{
public:
    virtual ~FConcordFactorHandleBase() {}

    virtual FFloatType ComputeScore(const FConcordExpressionContext<FFloatType>& Context) const = 0;
    virtual void AddScores(int32 NeighboringFlatRandomVariableIndex, const FConcordExpressionContextMutable<FFloatType>& Context, const TArrayView<FFloatType>& ScoresAcc) const = 0;

    struct FMaxSumVariableMessageElement { FFloatType Score; TMap<const FConcordFactorHandleBase*, TArray<int32>> NeighboringFactorOutwardValues; };
    using FMaxSumVariableMessage = TArray<FMaxSumVariableMessageElement>;
    virtual void AddMaxSumMessage(const FConcordExpressionContextMutable<FFloatType>& Context, TArray<FMaxSumVariableMessage>& Messages, int32 TargetFlatRandomVariableIndex) const = 0;

    template<typename FSumProductFloatType>
    using FVariableMessageFactors = TMap<const FConcordFactorHandleBase*, TArray<FSumProductFloatType>>;
    template<typename FSumProductFloatType>
    struct FSumProductMessages { TMap<const FConcordFactorHandleBase*, TArray<FSumProductFloatType>> FactorMessages; TArray<FVariableMessageFactors<FSumProductFloatType>> VariableMessageFactors; };
    virtual void SendSumProductMessage(const FConcordExpressionContextMutable<FFloatType>& Context, FSumProductMessages<FFloatType>& Messages, int32 TargetFlatRandomVariableIndex) const = 0;
    virtual void SendSumProductMessageDouble(const FConcordExpressionContextMutable<FFloatType>& Context, FSumProductMessages<double>& Messages, int32 TargetFlatRandomVariableIndex) const = 0;

    const TArray<int32>& GetNeighboringFlatRandomVariableIndices() const { return NeighboringFlatRandomVariableIndices; }
protected:
    TArray<int32> NeighboringFlatRandomVariableIndices;
};

using FConcordHandle = FConcordFactorHandleBase<float>;

template<typename FFactor, typename FFloatType>
class FConcordFactorHandle : public FConcordFactorHandleBase<FFloatType>
{
    using FMaxSumVariableMessage = typename FConcordFactorHandleBase<FFloatType>::FMaxSumVariableMessage;
    template<typename FSumProductFloatType>
    using FSumProductMessages = typename FConcordFactorHandleBase<FFloatType>::template FSumProductMessages<FSumProductFloatType>;
public:
    FFloatType ComputeScore(const FConcordExpressionContext<FFloatType>& Context) const override final
    {
        return static_cast<const FFactor*>(this)->Eval(Context);
    }

    void AddScores(int32 NeighboringFlatRandomVariableIndex, const FConcordExpressionContextMutable<FFloatType>& Context, const TArrayView<FFloatType>& ScoresAcc) const override
    {
        int32& Value = Context.Variation[NeighboringFlatRandomVariableIndex];
        for (Value = 0; Value < ScoresAcc.Num(); ++Value) ScoresAcc[Value] += ComputeScore(Context);
    }

    void AddMaxSumMessage(const FConcordExpressionContextMutable<FFloatType>& Context, TArray<FMaxSumVariableMessage>& Messages, int32 TargetFlatRandomVariableIndex) const override
    {
        TOptional<FFloatType> MaxScore;
        if (Context.ObservationMask[TargetFlatRandomVariableIndex])
        {
            AddMaxSumMessageImpl(Context, Messages, TargetFlatRandomVariableIndex, MaxScore);
            Messages[TargetFlatRandomVariableIndex][Context.Variation[TargetFlatRandomVariableIndex]].Score += MaxScore.GetValue();
            return;
        }
        int32& Value = Context.Variation[TargetFlatRandomVariableIndex];
        for (Value = 0; Value < Messages[TargetFlatRandomVariableIndex].Num(); ++Value)
        {
            AddMaxSumMessageImpl(Context, Messages, TargetFlatRandomVariableIndex, MaxScore);
            Messages[TargetFlatRandomVariableIndex][Context.Variation[TargetFlatRandomVariableIndex]].Score += MaxScore.GetValue();
            MaxScore.Reset();
        }
    }

    void SendSumProductMessage(const FConcordExpressionContextMutable<FFloatType>& Context, FSumProductMessages<FFloatType>& Messages, int32 TargetFlatRandomVariableIndex) const override
    {
        SendSumProductMessageWithType(Context, Messages, TargetFlatRandomVariableIndex);
    }

    void SendSumProductMessageDouble(const FConcordExpressionContextMutable<FFloatType>& Context, FSumProductMessages<double>& Messages, int32 TargetFlatRandomVariableIndex) const override
    {
        SendSumProductMessageWithType(Context, Messages, TargetFlatRandomVariableIndex);
    }
private:
    void AddMaxSumMessageImpl(const FConcordExpressionContextMutable<FFloatType>& Context, TArray<FMaxSumVariableMessage>& Messages, int32 TargetFlatRandomVariableIndex, TOptional<FFloatType>& MaxScore, int32 NeighborIndex = 0) const
    {
        if (NeighborIndex == NeighboringFlatRandomVariableIndices.Num())
        {        
            FFloatType Score = ComputeScore(Context);
            for (int32 NeighboringFlatRandomVariableIndex : NeighboringFlatRandomVariableIndices)
                if (NeighboringFlatRandomVariableIndex != TargetFlatRandomVariableIndex)
                    Score += Messages[NeighboringFlatRandomVariableIndex][Context.Variation[NeighboringFlatRandomVariableIndex]].Score;
            if (!MaxScore || MaxScore.GetValue() < Score)
            {
                MaxScore = Score;
                TArray<int32>& OutwardValues = Messages[TargetFlatRandomVariableIndex][Context.Variation[TargetFlatRandomVariableIndex]].NeighboringFactorOutwardValues[this];
                OutwardValues.Reset();
                for (int32 NeighboringFlatRandomVariableIndex : NeighboringFlatRandomVariableIndices)
                    if (NeighboringFlatRandomVariableIndex != TargetFlatRandomVariableIndex)
                        OutwardValues.Add(Context.Variation[NeighboringFlatRandomVariableIndex]);
            }
            return;
        }

        const int32 FlatRandomVariableIndex = NeighboringFlatRandomVariableIndices[NeighborIndex];
        if (FlatRandomVariableIndex == TargetFlatRandomVariableIndex || Context.ObservationMask[FlatRandomVariableIndex])
        {
            AddMaxSumMessageImpl(Context, Messages, TargetFlatRandomVariableIndex, MaxScore, NeighborIndex + 1);
            return;
        }
        int32& Value = Context.Variation[FlatRandomVariableIndex];
        for (Value = 0; Value < Messages[FlatRandomVariableIndex].Num(); ++Value)
            AddMaxSumMessageImpl(Context, Messages, TargetFlatRandomVariableIndex, MaxScore, NeighborIndex + 1);
    }

    template<typename FSumProductFloatType>
    void SendSumProductMessageWithType(const FConcordExpressionContextMutable<FFloatType>& Context, FSumProductMessages<FSumProductFloatType>& Messages, int32 TargetFlatRandomVariableIndex) const
    {
        if (Context.ObservationMask[TargetFlatRandomVariableIndex])
        {
            SendSumProductMessageImpl(Context, Messages, TargetFlatRandomVariableIndex, 0, Messages.FactorMessages[this].Num());
            return;
        }
        int32& Value = Context.Variation[TargetFlatRandomVariableIndex];
        for (Value = 0; Value < Messages.VariableMessageFactors[TargetFlatRandomVariableIndex][this].Num(); ++Value)
            SendSumProductMessageImpl(Context, Messages, TargetFlatRandomVariableIndex, 0, Messages.FactorMessages[this].Num());
    }

    template<typename FSumProductFloatType>
    void SendSumProductMessageImpl(const FConcordExpressionContextMutable<FFloatType>& Context, FSumProductMessages<FSumProductFloatType>& Messages, int32 TargetFlatRandomVariableIndex, int32 FactorMessageIndex, int32 FactorMessageStride, int32 NeighborIndex = 0) const
    {
        if (NeighborIndex == NeighboringFlatRandomVariableIndices.Num())
        {        
            FSumProductFloatType& FactorMessageValue = Messages.FactorMessages[this][FactorMessageIndex];
            FactorMessageValue = exp(FSumProductFloatType(ComputeScore(Context)));
            for (int32 NeighboringFlatRandomVariableIndex : NeighboringFlatRandomVariableIndices)
                if (NeighboringFlatRandomVariableIndex != TargetFlatRandomVariableIndex)
                    for (const auto& HandleValuesPair : Messages.VariableMessageFactors[NeighboringFlatRandomVariableIndex])
                        if (HandleValuesPair.Key != this)
                            FactorMessageValue *= HandleValuesPair.Value[Context.Variation[NeighboringFlatRandomVariableIndex]];
            Messages.VariableMessageFactors[TargetFlatRandomVariableIndex][this][Context.Variation[TargetFlatRandomVariableIndex]] += FactorMessageValue;
            return;
        }

        const int32 FlatRandomVariableIndex = NeighboringFlatRandomVariableIndices[NeighborIndex];
        const int32 StateCount = Messages.VariableMessageFactors[FlatRandomVariableIndex][this].Num();
        FactorMessageStride /= StateCount;
        if (FlatRandomVariableIndex == TargetFlatRandomVariableIndex || Context.ObservationMask[FlatRandomVariableIndex])
        {
            FactorMessageIndex += Context.Variation[FlatRandomVariableIndex] * FactorMessageStride;
            SendSumProductMessageImpl(Context, Messages, TargetFlatRandomVariableIndex, FactorMessageIndex, FactorMessageStride, NeighborIndex + 1);
        }
        else
        {
            int32& Value = Context.Variation[FlatRandomVariableIndex];
            for (Value = 0; Value < StateCount; ++Value)
            {
                SendSumProductMessageImpl(Context, Messages, TargetFlatRandomVariableIndex, FactorMessageIndex, FactorMessageStride, NeighborIndex + 1);
                FactorMessageIndex += FactorMessageStride;
            }
        }
    }
};
