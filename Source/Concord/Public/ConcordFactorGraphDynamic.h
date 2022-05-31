// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordExpression.h"
#include "FactorGraph/ConcordFactorGraph.h"

namespace ConcordFactorGraphDynamic
{
    struct FMergedHandle;

    struct CONCORD_API FHandle : FConcordFactorHandle<FHandle, float>
    {
        virtual float Eval(const FConcordExpressionContext<float>& Context) const = 0;
        virtual FMergedHandle* GetMergedHandle() { return nullptr; }
        virtual const FMergedHandle* GetMergedHandle() const { return nullptr; }
    };

    struct CONCORD_API FAtomicHandle : FHandle
    {
        FAtomicHandle(const FConcordSharedExpression& InFactor)
            : Factor(InFactor)
        {
            InFactor->AddNeighboringFlatRandomVariableIndices(NeighboringFlatRandomVariableIndices);
        }
        float Eval(const FConcordExpressionContext<float>& Context) const override
        {
            return Factor->ComputeValue(Context).Float;
        }
        const FConcordSharedExpression Factor;
    };

    struct CONCORD_API FMergedHandle : FHandle
    {
        FMergedHandle() {}
        FMergedHandle(const FMergedHandle&) = delete;
        FMergedHandle& operator=(const FMergedHandle&) = delete;
        float Eval(const FConcordExpressionContext<float>& Context) const override
        {
            float Score = 0.0f;
            for (const TUniquePtr<FAtomicHandle>& Handle : Children)
                Score += Handle->Eval(Context);
            return Score;
        }
        FMergedHandle* GetMergedHandle() override { return this; }
        const FMergedHandle* GetMergedHandle() const override { return this; }
        void AddHandle(TUniquePtr<FAtomicHandle> InHandle)
        {
            for (int32 FlatRandomVariableIndex : InHandle->GetNeighboringFlatRandomVariableIndices())
                NeighboringFlatRandomVariableIndices.AddUnique(FlatRandomVariableIndex);
            Children.Add(MoveTemp(InHandle));
        }
        TArray<TUniquePtr<FAtomicHandle>> Children;
    };

    struct CONCORD_API FOutput : FConcordFactorGraph<float>::FOutput
    {
        FOutput(EConcordValueType InType, TArray<FConcordSharedExpression>&& InExpressions)
            : FConcordFactorGraph<float>::FOutput(InType)
            , Expressions(MoveTemp(InExpressions))
        {}
        void Eval(const FConcordExpressionContext<float>& Context, const TArrayView<int32>& OutData) const override
        {
            check(Expressions.Num() >= OutData.Num());
            for (int32 Index = 0; Index < OutData.Num(); ++Index) OutData[Index] = Expressions[Index]->ComputeValue(Context).Int;
        }
        void Eval(const FConcordExpressionContext<float>& Context, const TArrayView<float>& OutData) const override
        {
            check(Expressions.Num() >= OutData.Num());
            for (int32 Index = 0; Index < OutData.Num(); ++Index) OutData[Index] = Expressions[Index]->ComputeValue(Context).Float;
        }
        int32 Num() const override { return Expressions.Num(); }
        const TArray<FConcordSharedExpression> Expressions;
    };
};
