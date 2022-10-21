// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerPR.generated.h"

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
    int32 GetDegreesOffset() const { return 1 + FImpl::NumAdditionalSourceExpressions; }
    int32 GetNumDegrees() const { return SourceExpressions.Num() - GetDegreesOffset(); }
    int32 GetDegree(int32 Index, const FConcordExpressionContext<float>& Context) const
    {
        check(Index >= 0 && Index < GetNumDegrees());
        return SourceExpressions[Index + GetDegreesOffset()]->ComputeValue(Context).Int;
    }

    template<typename FF>
    bool StepBackward(FF F, int& Index, const FConcordExpressionContext<float>& Context) const
    {
        for (--Index; Index >= 0; --Index) if (!F(GetDegree(Index, Context))) return false;
        return true;
    }

    template<typename FF>
    bool StepForward(FF F, int& Index, const FConcordExpressionContext<float>& Context) const
    {
        for (++Index; Index < GetNumDegrees(); ++Index) if (!F(GetDegree(Index, Context))) return false;
        return true;
    }
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
protected:
    int32 GetBoundaryIndex(const FConcordExpressionContext<float>& Context) const
    {
        return FBase::SourceExpressions[1]->ComputeValue(Context).Int;
    }
};

class FConcordGPR1Expression : public FConcordPRExpression<FConcordGPR1Expression>
{
public:
    using FBase = FConcordPRExpression<FConcordGPR1Expression>;
    using FBase::FBase;
    static inline int NumAdditionalSourceExpressions = 0;
    int32 ComputeScoreClass(const FConcordExpressionContext<float>& Context) const;
};

class FConcordGPR2bExpression : public FConcordPR2GroupsExpression<FConcordGPR2bExpression>
{
public:
    using FBase = FConcordPR2GroupsExpression<FConcordGPR2bExpression>;
    using FBase::FBase;
    int32 ComputeScoreClass(const FConcordExpressionContext<float>& Context) const;
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
        InputInfo.Add({ {}, EConcordValueType::Float, INVTEXT("Scale"), "0" });
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

UCLASS()
class CONCORD_API UConcordTransformerGPR2b : public UConcordTransformerPR
{
    GENERATED_BODY()
public:
    FText GetDisplayName() const override { return INVTEXT("GPR2b"); }
    FText GetTooltip() const override { return INVTEXT("Tries to apply grouping preference rule 2b before the boundary note given in scale degrees."); }
private:
    virtual void AddAdditionalInputs(TArray<FInputInfo>& InOutInputInfo) const
    {
        InOutInputInfo.Add({ {}, EConcordValueType::Int, INVTEXT("Boundary Degree Index"), "0" });
    }

    FConcordSharedExpression MakeExpression(TArray<FConcordSharedExpression>&& SourceExpressions) const override
    {
        return MakeShared<const FConcordGPR2bExpression>(MoveTemp(SourceExpressions));
    }
};
