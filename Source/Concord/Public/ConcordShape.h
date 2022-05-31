// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

using FConcordShape = TArray<int32>;
using FConcordMultiIndex = TArray<int32>;

namespace ConcordShape
{
    inline bool IsValidShape(const FConcordShape& Shape)
    {
        if (Shape.IsEmpty()) return false;
        for (int32 DimSize : Shape)
            if (DimSize < 1) return false;
        return true;
    }

    inline int32 GetFlatNum(const FConcordShape& Shape)
    {
        int32 FlatNum = 1;
        for (int32 DimSize : Shape)
            FlatNum *= DimSize;
        return FlatNum;
    }

    inline int32 FlattenIndex(const FConcordMultiIndex& MultiIndex, const FConcordShape& Shape)
    {
        int32 FlatIndex = 0;
        int32 Stride = 1;
        for (int32 DimIndex = Shape.Num() - 1; DimIndex >= 0; --DimIndex)
        {
            FlatIndex += MultiIndex[DimIndex] * Stride;
            Stride *= Shape[DimIndex];
        }
        return FlatIndex;
    }

    inline FConcordMultiIndex UnflattenIndex(int32 FlatIndex, const FConcordShape& Shape)
    {
        FConcordMultiIndex MultiIndex;
        MultiIndex.AddUninitialized(Shape.Num());
        for (int32 DimIndex = Shape.Num() - 1; DimIndex >= 0; --DimIndex)
        {
            MultiIndex[DimIndex] = FlatIndex % Shape[DimIndex];
            FlatIndex /= Shape[DimIndex];
        }
        return MoveTemp(MultiIndex);
    }

    inline TOptional<FConcordShape> Broadcast(const TArray<FConcordShape>& Shapes)
    {
        FConcordShape BroadcastedShape;
        for (const FConcordShape& Shape : Shapes)
        {
            for (int32 DimIndex = 0; DimIndex < Shape.Num(); ++DimIndex)
            {
                if (DimIndex >= BroadcastedShape.Num())
                    BroadcastedShape.Add(Shape[DimIndex]);
                else if (BroadcastedShape[DimIndex] == 1)
                    BroadcastedShape[DimIndex] = Shape[DimIndex];
                else if (Shape[DimIndex] != 1 && BroadcastedShape[DimIndex] != Shape[DimIndex])
                    return {};
            }
        }
        return BroadcastedShape;
    }

    inline bool AreShapesCompatible(const TArray<FConcordShape>& Shapes)
    {
        return Broadcast(Shapes).IsSet();
    }

    inline void Unbroadcast(FConcordMultiIndex& MultiIndex, const FConcordShape& Shape)
    {
        check(MultiIndex.Num() >= Shape.Num());
        MultiIndex.SetNum(Shape.Num());
        for (int32 DimIndex = 0; DimIndex < Shape.Num(); ++DimIndex)
            if (Shape[DimIndex] == 1) MultiIndex[DimIndex] = 0;
    }

    struct FShapeIterator
    {
        FShapeIterator(const FConcordShape& InShape)
            : Shape(InShape)
        {
            Step.AddZeroed(InShape.Num());
        }

        bool HasNext() const { return Step.Num() > 0 && Step[0] < Shape[0]; }

        FConcordMultiIndex Next()
        {
            FConcordMultiIndex Ret = Step;
            IncrementStep(Step.Num() - 1);
            return MoveTemp(Ret);
        }

    private:
        FConcordShape Shape;
        FConcordMultiIndex Step;

        void IncrementStep(int32 DimIndex)
        {
            if (++Step[DimIndex] == Shape[DimIndex] && DimIndex > 0)
            {
                Step[DimIndex] = 0;
                IncrementStep(DimIndex - 1);
            }
        }
    };

    inline bool IsRestZero(const FConcordMultiIndex& MultiIndex, int32 InDimIndex)
    {
        for (int32 DimIndex = InDimIndex + 1; DimIndex < MultiIndex.Num(); ++DimIndex)
            if (MultiIndex[DimIndex] != 0) return false;
        return true;
    }

    inline FString ToString(const TArray<int32>& IntArray)
    {
        FString String; String.Reserve(IntArray.Num() * 4);
        String = TEXT("(");
        for (int32 Value : IntArray) String += FString::Printf(TEXT("%i, "), Value);
        if (!IntArray.IsEmpty()) String.LeftChopInline(2);
        String += TEXT(")");
        return MoveTemp(String);
    }
}
