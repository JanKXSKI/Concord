// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "Transformers/ConcordTransformerConcat.h"

FString UConcordTransformerConcat::GetCategory() const
{
    return TEXT("Shape");
}

FText UConcordTransformerConcat::GetDisplayName() const
{
    return INVTEXT("Concat");
}

FText UConcordTransformerConcat::GetTooltip() const
{
    return INVTEXT("Concatenates its inputs, flattening when necessary.");
}

FConcordSharedExpression UConcordTransformerConcat::GetExpression(const FConcordMultiIndex& MultiIndex) const
{
    switch (Mode)
    {
    case Flat:
    {
        check(MultiIndex.Num() == 1);
        int32 FlatIndex = MultiIndex[0];
        for (int32 ConnectedIndex = 0; ConnectedIndex < ConnectedShapeFlatNums.Num(); ++ConnectedIndex)
        {
            int32 FlatNum = ConnectedShapeFlatNums[ConnectedIndex];
            if (FlatIndex >= FlatNum) { FlatIndex -= FlatNum; continue; }
            const UConcordTransformer* ConnectedTransformer = GetConnectedTransformers()[ConnectedIndex];
            return ConnectedTransformer->GetExpression(ConcordShape::UnflattenIndex(FlatIndex, ConnectedTransformer->GetShape()));
        }
        checkNoEntry(); return MakeShared<const FConcordValueExpression<int32>>(0);
    }
    case FlatArguments:
    {
        check(MultiIndex.Num() == 2);
        const UConcordTransformer* ConnectedTransformer = GetConnectedTransformers()[MultiIndex[0]];
        return ConnectedTransformer->GetExpression(ConcordShape::UnflattenIndex(MultiIndex[1], ConnectedTransformer->GetShape()));
    }
    case Stack:
    {
        if (MultiIndex.Num() == 1) return GetConnectedTransformers()[MultiIndex[0]]->GetExpression({ 0 });
        FConcordMultiIndex NextMultiIndex = MultiIndex;
        NextMultiIndex.RemoveAt(0, 1, false);
        return GetConnectedTransformers()[MultiIndex[0]]->GetExpression(NextMultiIndex);
    }
    }
    checkNoEntry(); return MakeShared<const FConcordValueExpression<int32>>(0);
}

UConcordVertex::FSetupResult UConcordTransformerConcat::Setup(TOptional<FString>& OutErrorMessage)
{
    return { ComputeShape(OutErrorMessage), ComputeType(OutErrorMessage) };
}

TArray<int32> UConcordTransformerConcat::ComputeShape(TOptional<FString>& OutErrorMessage)
{
    TArray<FConcordShape> ConnectedShapes = GetConnectedShapes();
    if (ConnectedShapes.Num() == 0) { return {}; }

    Mode = Stack;
    const FConcordShape& FirstShape = ConnectedShapes[0];
    for (int32 Index = 1; Index < ConnectedShapes.Num(); ++Index)
        if (FirstShape != ConnectedShapes[Index]) { Mode = Flat; break; }
    if (Mode == Flat)
    {
        ConnectedShapeFlatNums.Empty(ConnectedShapes.Num());
        Mode = FlatArguments;
        int32 FlatSize = 0;
        for (const FConcordShape& ConnectedShape : ConnectedShapes)
        {
            ConnectedShapeFlatNums.Add(ConcordShape::GetFlatNum(ConnectedShape));
            FlatSize += ConnectedShapeFlatNums.Last();
            if (ConnectedShapeFlatNums.Num() > 1 && ConnectedShapeFlatNums.Last() != ConnectedShapeFlatNums[0]) Mode = Flat;
        }
        if (Mode == Flat) return { FlatSize };
        else return {  ConnectedShapes.Num(), ConnectedShapeFlatNums[0] };
    }
    else
    {
        TArray<int32> StackedShape = { ConnectedShapes.Num() };
        if (FirstShape.Num() > 1 || FirstShape[0] != 1)
            StackedShape.Append(FirstShape);
        return StackedShape;
    }
}

EConcordValueType UConcordTransformerConcat::ComputeType(TOptional<FString>& OutErrorMessage)
{
    EConcordValueType FirstType = GetConnectedTransformers()[0]->GetType();
    for (int32 Index = 1; Index < GetConnectedTransformers().Num(); ++Index)
        if (GetConnectedTransformers()[Index]->GetType() != FirstType)
        {
            OutErrorMessage = TEXT("Concat inputs have different types.");
            return EConcordValueType::Error;
        }
    return FirstType;
}
