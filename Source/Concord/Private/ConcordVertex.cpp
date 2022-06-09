// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "ConcordVertex.h"
#include "ConcordTransformer.h"
#include "ConcordBox.h"
#include "ConcordComposite.h"
#include "ConcordInstance.h"
#include "ConcordParameter.h"
#include "ConcordModel.h"
#include "Transformers/ConcordTransformerCast.h"
#include "Transformers/ConcordTransformerTruncate.h"
#include "Transformers/ConcordTransformerValue.h"

TOptional<FConcordError> UConcordVertex::SetupGraph(TSet<UConcordVertex*>& VisitedVertices)
{
    if (VisitedVertices.Contains(this)) return {};

    Shape = {};
    Type = EConcordValueType::Error;

    for (UConcordTransformer* ConnectedTransformer : ConnectedTransformers)
        if (TOptional<FConcordError> Error = ConnectedTransformer->SetupGraph(VisitedVertices))
            return Error;

    for (int32 Index = 0; Index < GetInputInfo().Num(); ++Index)
    {
        if (GetInputInfo()[Index].Shape && GetInputInfo()[Index].Shape.GetValue() != ConnectedTransformers[Index]->GetShape())
            return FConcordError { this, FString::Printf(TEXT("Connection to input %s has bad shape, expected %s."), *GetInputInfo()[Index].DisplayName.ToString(), *ConcordShape::ToString(GetInputInfo()[Index].Shape.GetValue())) };
        if (GetInputInfo()[Index].Type && GetInputInfo()[Index].Type.GetValue() != ConnectedTransformers[Index]->GetType())
            return FConcordError { this, FString::Printf(TEXT("Connection to input %s has bad type."), *GetInputInfo()[Index].DisplayName.ToString()) };
    }

    TOptional<FString> ErrorMessage;
    FSetupResult SetupResult = Setup(ErrorMessage);
    if (ErrorMessage) return FConcordError { this, MoveTemp(ErrorMessage.GetValue()) };
    Shape = SetupResult.Shape;
    Type = SetupResult.Type;

    VisitedVertices.Add(this);
    return {};
}

TArray<FConcordShape> UConcordVertex::GetConnectedShapes() const
{
    TArray<FConcordShape> ConnectedShapes; ConnectedShapes.Reserve(ConnectedTransformers.Num());
    for (UConcordTransformer* ConnectedTransformer : ConnectedTransformers)
        ConnectedShapes.Add(ConnectedTransformer->GetShape());
    return MoveTemp(ConnectedShapes);
}

FConcordShape UConcordVertex::ComputeBroadcastedShape(TOptional<FString>& OutErrorMessage) const
{
    TOptional<FConcordShape> BroadcastedShape = ConcordShape::Broadcast(GetConnectedShapes());
    if (BroadcastedShape) return BroadcastedShape.GetValue();
    OutErrorMessage = TEXT("Input shapes could not be broadcasted together.");
    return {};
}

TArray<FConcordSharedExpression> UConcordVertex::GetBroadcastedConnectedExpressions(const FConcordMultiIndex& MultiIndex, TOptional<EConcordValueType> TargetType) const
{
    TArray<FConcordSharedExpression> ConnectedExpressions; ConnectedExpressions.Reserve(GetConnectedTransformers().Num());
    AddBroadcastedConnectedExpressions(ConnectedExpressions, 0, GetConnectedTransformers().Num(), MultiIndex, TargetType);
    return MoveTemp(ConnectedExpressions);
}

void UConcordVertex::AddBroadcastedConnectedExpressions(TArray<FConcordSharedExpression>& OutExpressions, int32 BeginIndex, int32 Count, const FConcordMultiIndex& MultiIndex, TOptional<EConcordValueType> TargetType) const
{
    for (int32 Index = BeginIndex; Index < BeginIndex + Count; ++Index)
    {
        const UConcordTransformer* ConnectedTransformer = GetConnectedTransformers()[Index];
        FConcordMultiIndex ConnectedMultiIndex = MultiIndex;
        ConcordShape::Unbroadcast(ConnectedMultiIndex, ConnectedTransformer->GetShape());
        OutExpressions.Add(GetConnectedExpression(ConnectedTransformer, ConnectedMultiIndex, TargetType.Get(ConnectedTransformer->GetType())));
    }
}

FConcordSharedExpression UConcordVertex::GetConnectedExpression(const UConcordTransformer* Transformer, const FConcordMultiIndex& MultiIndex, EConcordValueType TargetType)
{
    FConcordSharedExpression ConnectedExpression = Transformer->GetExpression(MultiIndex);
    check(Transformer->GetType() == EConcordValueType::Float || Transformer->GetType() == EConcordValueType::Int);
    if (Transformer->GetType() == TargetType) return MoveTemp(ConnectedExpression);
    if (TargetType == EConcordValueType::Float)
        ConnectedExpression = MakeShared<const FConcordCastExpression>(MoveTemp(ConnectedExpression));
    else if (TargetType == EConcordValueType::Int)
        ConnectedExpression = MakeShared<const FConcordTruncateExpression>(MoveTemp(ConnectedExpression));
    return MoveTemp(ConnectedExpression);
}

EConcordValueType UConcordVertex::GetSumType() const
{
    if (ConnectedTransformers.FindByPredicate([](const UConcordTransformer* Transformer){ return Transformer->GetType() != EConcordValueType::Float && Transformer->GetType() != EConcordValueType::Int; }))
        return EConcordValueType::Error;
    for (const UConcordTransformer* ConnectedTransformer : ConnectedTransformers)
        if (ConnectedTransformer->GetType() == EConcordValueType::Float)
            return EConcordValueType::Float;
    return EConcordValueType::Int;
}

bool UConcordVertex::IsPinDefaultValue() const
{
    if (const UConcordTransformerValue* TransformerValue = Cast<const UConcordTransformerValue>(this))
        return TransformerValue->bIsPinDefaultValue;
    return false;
}

void UConcordVertex::GetUpstreamSources(TArray<UConcordVertex*>& OutUpstreamSources)
{
    if (OutUpstreamSources.Contains(this)) return;

    for (UConcordTransformer* ConnectedTransformer : ConnectedTransformers)
        ConnectedTransformer->GetUpstreamSources(OutUpstreamSources);
    if (UConcordCompositeOutput* CompositeOutput = Cast<UConcordCompositeOutput>(this))
        CompositeOutput->GetComposite()->GetUpstreamSources(OutUpstreamSources);
    if (UConcordInstanceOutput* InstanceOutput = Cast<UConcordInstanceOutput>(this))
        InstanceOutput->GetInstance()->GetUpstreamSources(OutUpstreamSources);

    else if (IsA<UConcordBox>() || IsA<UConcordComposite>() || IsA<UConcordInstance>() || IsA<UConcordParameter>())
        OutUpstreamSources.Add(this);
}
