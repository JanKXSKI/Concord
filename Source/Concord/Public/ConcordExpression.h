// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordValue.h"
#include "ConcordExpressionContext.h"

class FConcordRandomVariableExpression;
class FConcordComputingExpression;
template<typename FValue> class FConcordParameterExpression;
template<typename FValue> class FConcordValueExpression;

class FConcordExpression
{
public:
    virtual ~FConcordExpression() {}
    virtual const FConcordRandomVariableExpression* AsRandomVariableExpression() const { return nullptr; }
    virtual const FConcordParameterExpression<int32>* AsIntParameterExpression() const { return nullptr; }
    virtual const FConcordParameterExpression<float>* AsFloatParameterExpression() const { return nullptr; }
    virtual const FConcordComputingExpression* AsComputingExpression() const { return nullptr; }
    virtual const FConcordValueExpression<int32>* AsIntValueExpression() const { return nullptr; }
    virtual const FConcordValueExpression<float>* AsFloatValueExpression() const { return nullptr; }
    virtual FConcordValue ComputeValue(const FConcordExpressionContext<float>& Context) const = 0;
    virtual void AddNeighboringFlatRandomVariableIndices(TArray<int32>& OutNeighboringFlatRandomVariableIndices) const {}
};

using FConcordSharedExpression = TSharedRef<const FConcordExpression>;

class FConcordBlockExpression : public FConcordExpression
{
public:
    FConcordBlockExpression(int32 InFlatIndex)
        : FlatIndex(InFlatIndex)
    {}
    const int32 FlatIndex;
};

class FConcordRandomVariableExpression : public FConcordBlockExpression
{
public:
    FConcordRandomVariableExpression(int32 InFlatIndex)
        : FConcordBlockExpression(InFlatIndex)
    {}
    const FConcordRandomVariableExpression* AsRandomVariableExpression() const override { return this; }
    FConcordValue ComputeValue(const FConcordExpressionContext<float>& Context) const override { return Context.Variation[FlatIndex]; }
    void AddNeighboringFlatRandomVariableIndices(TArray<int32>& OutNeighboringFlatRandomVariableIndices) const override
    {
        OutNeighboringFlatRandomVariableIndices.AddUnique(FlatIndex);
    }
};

using FConcordSharedRandomVariableExpression = TSharedRef<const FConcordRandomVariableExpression>;

template<> class FConcordParameterExpression<int32> : public FConcordBlockExpression
{
public:
    FConcordParameterExpression(int32 InFlatIndex)
        : FConcordBlockExpression(InFlatIndex)
    {}
    const FConcordParameterExpression<int32>* AsIntParameterExpression() const override { return this; }
    FConcordValue ComputeValue(const FConcordExpressionContext<float>& Context) const override { return Context.IntParameters[FlatIndex]; }
};

template<> class FConcordParameterExpression<float> : public FConcordBlockExpression
{
public:
    FConcordParameterExpression(int32 InFlatIndex)
        : FConcordBlockExpression(InFlatIndex)
    {}
    const FConcordParameterExpression<float>* AsFloatParameterExpression() const override { return this; }
    FConcordValue ComputeValue(const FConcordExpressionContext<float>& Context) const override { return Context.FloatParameters[FlatIndex]; }
};

template<typename FValue> using FConcordSharedParameterExpression = TSharedRef<const FConcordParameterExpression<FValue>>;

class FConcordComputingExpression : public FConcordExpression
{
public:
    FConcordComputingExpression(TArray<FConcordSharedExpression>&& InSourceExpressions)
        : SourceExpressions(MoveTemp(InSourceExpressions))
    {}
    const TArray<FConcordSharedExpression> SourceExpressions;
    const FConcordComputingExpression* AsComputingExpression() const override { return this; }
#if WITH_EDITOR
    virtual FString ToString() const = 0;
#endif
    void AddNeighboringFlatRandomVariableIndices(TArray<int32>& OutNeighboringFlatRandomVariableIndices) const override
    {
        for (const FConcordSharedExpression& SourceExpression : SourceExpressions)
            SourceExpression->AddNeighboringFlatRandomVariableIndices(OutNeighboringFlatRandomVariableIndices);
    }
};

template<> class FConcordValueExpression<int32> : public FConcordExpression
{
public:
    FConcordValueExpression(int32 InValue) : Value(InValue) {}
    const FConcordValueExpression<int32>* AsIntValueExpression() const override { return this; }
    FConcordValue ComputeValue(const FConcordExpressionContext<float>& Context) const override { return Value; }
    const int32 Value;
};

template<> class FConcordValueExpression<float> : public FConcordExpression
{
public:
    FConcordValueExpression(float InValue) : Value(InValue) {}
    const FConcordValueExpression<float>* AsFloatValueExpression() const override { return this; }
    FConcordValue ComputeValue(const FConcordExpressionContext<float>& Context) const override { return Value; }
    const float Value;
};
