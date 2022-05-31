// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordShape.h"
#include "ConcordExpression.h"
#include "ConcordError.h"
#include "ConcordValue.h"
#include "UObject/Object.h"
#include "ConcordVertex.generated.h"

class UConcordTransformer;

UCLASS(Abstract, CollapseCategories)
class CONCORD_API UConcordVertex : public UObject
{
    GENERATED_BODY()
public:
    UConcordVertex() : Type(EConcordValueType::Error) {}

    virtual FText GetDisplayName() const { return FText::FromString(GetClass()->GetName()); }
    virtual FText GetNodeDisplayName() const { return GetDisplayName(); }
    virtual FText GetTooltip() const { return INVTEXT("A vertex with a shape and a type that can create expressions."); }

    struct FInputInfo { TOptional<FConcordShape> Shape; TOptional<EConcordValueType> Type; FText DisplayName; FString Default; };
    virtual TArray<FInputInfo> GetInputInfo() const { return {}; }

    const FConcordShape& GetShape() const { return Shape; }
    EConcordValueType GetType() const { return Type; }
    const TArray<UConcordTransformer*>& GetConnectedTransformers() const { return ConnectedTransformers; }

    FConcordShape ComputeBroadcastedShape(TOptional<FString>& OutErrorMessage) const;
    TArray<FConcordSharedExpression> GetBroadcastedConnectedExpressions(const FConcordMultiIndex& MultiIndex, TOptional<EConcordValueType> TargetType = {}) const;
    void AddBroadcastedConnectedExpressions(TArray<FConcordSharedExpression>& OutExpressions, int32 BeginIndex, int32 Count, const FConcordMultiIndex& MultiIndex, TOptional<EConcordValueType> TargetType = {}) const;
    static FConcordSharedExpression GetConnectedExpression(const UConcordTransformer* Transformer, const FConcordMultiIndex& MultiIndex, EConcordValueType TargetType);
    EConcordValueType GetSumType() const;
    bool IsPinDefaultValue() const;

    TOptional<FConcordError> SetupGraph(TSet<UConcordVertex*>& VisitedVertices);
    void GetUpstreamSources(TArray<UConcordVertex*>& OutUpstreamSources);
protected:
    TArray<FConcordShape> GetConnectedShapes() const;
    struct FSetupResult { FConcordShape Shape; EConcordValueType Type; };
private:    
    UPROPERTY()
    TArray<UConcordTransformer*> ConnectedTransformers;

    UPROPERTY(VisibleAnywhere, Category = "Vertex")
    TArray<int32> Shape;

    UPROPERTY(VisibleAnywhere, Category = "Vertex")
    EConcordValueType Type;

    // Calling GetExpression() on ConnectedTransformers in Setup() is an Error!
    virtual FSetupResult Setup(TOptional<FString>& OutErrorMessage) PURE_VIRTUAL(UConcordVertex::Setup, return {};)

    friend class FConcordCompiler;
#if WITH_EDITOR
    friend class UConcordModelGraphConsumer;
    friend class UConcordModelGraphModelNode;
    friend class FConcordModelGraphSync;
#endif // WITH_EDITOR
};

UCLASS()
class CONCORD_API UConcordMissingVertex : public UConcordVertex
{
    GENERATED_BODY()
public:
    virtual FText GetDisplayName() const { return INVTEXT("Missing Vertex."); }
    virtual FText GetTooltip() const { return INVTEXT("A copy of a node from another graph that references a Concord vertex type that is not available."); }
private:
    FSetupResult Setup(TOptional<FString>& OutErrorMessage) override { OutErrorMessage = TEXT("This vertex is missing!"); return {}; }
};
