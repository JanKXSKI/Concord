// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordTransformer.h"
#include "ConcordTransformerInterpolate.generated.h"

class FConcordInterpolateExpression : public FConcordComputingExpression
{
public:
    FConcordInterpolateExpression(TArray<FConcordSharedExpression>&& InSourceExpressions)
        : FConcordComputingExpression(MoveTemp(InSourceExpressions))
    {}
protected:
    int32 GetIndex(int32 Offset, int32 End, const FConcordExpressionContext<float>& Context) const;
    FString GetIndexToString(const TCHAR* Suffix, int32 Offset, int32 End) const;
};

template<typename FValue>
class FConcordLinearInterpolateExpression : public FConcordInterpolateExpression
{
public:
    FConcordLinearInterpolateExpression(TArray<FConcordSharedExpression>&& InSourceExpressions, int32 InForwardCount)
        : FConcordInterpolateExpression(MoveTemp(InSourceExpressions))
        , ForwardCount(InForwardCount)
    {}
    FConcordValue ComputeValue(const FConcordExpressionContext<float>& Context) const override;
#if WITH_EDITOR
    FString ToString() const override;
#endif
private:
    const int32 ForwardCount;
};

class FConcordConstantInterpolateExpression : public FConcordInterpolateExpression
{
public:
    FConcordConstantInterpolateExpression(TArray<FConcordSharedExpression>&& InSourceExpressions)
        : FConcordInterpolateExpression(MoveTemp(InSourceExpressions))
    {}
    FConcordValue ComputeValue(const FConcordExpressionContext<float>& Context) const override;
#if WITH_EDITOR
    FString ToString() const override;
#endif
};

template<bool bBackward>
class FConcordConstantBidirectionalInterpolateExpression : public FConcordInterpolateExpression
{
public:
    FConcordConstantBidirectionalInterpolateExpression(TArray<FConcordSharedExpression>&& InSourceExpressions, int32 InForwardCount)
        : FConcordInterpolateExpression(MoveTemp(InSourceExpressions))
        , ForwardCount(InForwardCount)
    {}
    FConcordValue ComputeValue(const FConcordExpressionContext<float>& Context) const override;
#if WITH_EDITOR
    FString ToString() const override;
#endif
private:
    const int32 ForwardCount;
};

UENUM()
enum class EConcordTransformerInterpolateMode : uint8
{
    Linear                      UMETA(DisplayName = "Linear"),
    ConstantForward             UMETA(DisplayName = "Constant (Forward)"),
    ConstantBackward            UMETA(DisplayName = "Constant (Backward)"),
    ConstantForwardBackward     UMETA(DisplayName = "Constant (Forward Backward)"),
    ConstantBackwardForward     UMETA(DisplayName = "Constant (Backward Forward)")
};

UCLASS()
class CONCORD_API UConcordTransformerInterpolate : public UConcordTransformer
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Interpolate")
    int32 DimensionIndex;

    UPROPERTY(EditAnywhere, Category = "Interpolate")
    EConcordTransformerInterpolateMode Mode;

    UPROPERTY(EditAnywhere, Category = "Interpolate", meta = (MinValue = 1))
    int32 MaxDistance;

    UConcordTransformerInterpolate()
        : DimensionIndex(-1)
        , Mode(EConcordTransformerInterpolateMode::ConstantForwardBackward)
        , MaxDistance(16)
        , ActualDimensionIndex(0)
    {}
    
    TArray<FInputInfo> GetInputInfo() const override
    {
        return
        {
            { {}, {}, INVTEXT("Values"), "0" },
            { {}, EConcordValueType::Int, INVTEXT("Mask"), "0" }
        };
    }

    FString GetCategory() const override { return TEXT("Sequence"); }

    FText GetDisplayName() const override { return INVTEXT("Interpolate"); }
    FText GetTooltip() const override { return INVTEXT("Interpolates the values where Mask is 1 (true) into the spaces where Mask is 0 (false)."); }

    FConcordSharedExpression GetExpression(const FConcordMultiIndex& InMultiIndex) const override
    {
        if (GetConnectedTransformers()[0]->GetShape()[ActualDimensionIndex] == 1) // trivial case handled explicitly, assume at least 2 values available otherwise
        {
            FConcordMultiIndex ConnectedMultiIndex = InMultiIndex;
            ConcordShape::Unbroadcast(ConnectedMultiIndex, GetConnectedTransformers()[0]->GetShape());
            return GetConnectedTransformers()[0]->GetExpression(ConnectedMultiIndex);
        }

        TArray<FConcordSharedExpression> SourceExpressions;
        int32 ForwardCount = 0;
        for (int32 Index = 0; Index < 2; ++Index)
        {
            FConcordMultiIndex ConnectedMultiIndex = InMultiIndex;
            ConcordShape::Unbroadcast(ConnectedMultiIndex, GetConnectedTransformers()[Index]->GetShape());
            const int32 DimIndex = ConnectedMultiIndex[ActualDimensionIndex];
            const int32 DimSize = GetConnectedTransformers()[0]->GetShape()[ActualDimensionIndex];
            switch (Mode)
            {
            case EConcordTransformerInterpolateMode::ConstantForward:
                for (int32 Step = 0; Step <= MaxDistance && ConnectedMultiIndex[ActualDimensionIndex] >= 0; ++Step)
                {
                    SourceExpressions.Add(GetConnectedTransformers()[Index]->GetExpression(ConnectedMultiIndex));
                    --ConnectedMultiIndex[ActualDimensionIndex];
                }
                break;
            case EConcordTransformerInterpolateMode::ConstantBackward:
                for (int32 Step = 0; Step <= MaxDistance && ConnectedMultiIndex[ActualDimensionIndex] < DimSize; ++Step)
                {
                    SourceExpressions.Add(GetConnectedTransformers()[Index]->GetExpression(ConnectedMultiIndex));
                    ++ConnectedMultiIndex[ActualDimensionIndex];
                }
                break;
            default:
                for (int32 Step = 0; Step <= MaxDistance && ConnectedMultiIndex[ActualDimensionIndex] >= 0; ++Step)
                {
                    SourceExpressions.Add(GetConnectedTransformers()[Index]->GetExpression(ConnectedMultiIndex));
                    --ConnectedMultiIndex[ActualDimensionIndex];
                    if (Index == 0) ++ForwardCount;
                }
                ConnectedMultiIndex[ActualDimensionIndex] = DimIndex + 1;
                for (int32 Step = 1; Step <= MaxDistance && ConnectedMultiIndex[ActualDimensionIndex] < DimSize; ++Step)
                {
                    SourceExpressions.Add(GetConnectedTransformers()[Index]->GetExpression(ConnectedMultiIndex));
                    ++ConnectedMultiIndex[ActualDimensionIndex];
                }
                break;
            }
        }

        switch (Mode)
        {
        case EConcordTransformerInterpolateMode::Linear:
            switch (GetType())
            {
            case EConcordValueType::Int: return MakeShared<const FConcordLinearInterpolateExpression<int32>>(MoveTemp(SourceExpressions), ForwardCount);
            case EConcordValueType::Float: return MakeShared<const FConcordLinearInterpolateExpression<float>>(MoveTemp(SourceExpressions), ForwardCount);
            default: checkNoEntry(); return MakeShared<const FConcordValueExpression<int32>>(0);
            }
        case EConcordTransformerInterpolateMode::ConstantForward:
        case EConcordTransformerInterpolateMode::ConstantBackward:
            return MakeShared<const FConcordConstantInterpolateExpression>(MoveTemp(SourceExpressions));
        case EConcordTransformerInterpolateMode::ConstantForwardBackward:
            return MakeShared<const FConcordConstantBidirectionalInterpolateExpression<false>>(MoveTemp(SourceExpressions), ForwardCount);
        case EConcordTransformerInterpolateMode::ConstantBackwardForward:
            return MakeShared<const FConcordConstantBidirectionalInterpolateExpression<true>>(MoveTemp(SourceExpressions), ForwardCount);
        default:
            checkNoEntry(); return MakeShared<const FConcordValueExpression<int32>>(0);
        }
    }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override
    {
        if (!GetActualDimensionIndex(DimensionIndex, GetConnectedTransformers()[0]->GetShape(), ActualDimensionIndex, OutErrorMessage)) return {};
        const auto Shapes = GetConnectedShapes();
        if (Shapes[0].Num() != Shapes[1].Num() || Shapes[0][ActualDimensionIndex] != Shapes[1][ActualDimensionIndex])
        {
            OutErrorMessage = TEXT("Dimension count and size of given dimension of Values and Mask must match.");
            return {};
        }
        return { ComputeBroadcastedShape(OutErrorMessage), GetConnectedTransformers()[0]->GetType() };
    }

    int32 ActualDimensionIndex;
};
