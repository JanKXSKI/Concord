 // Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordVertex.h"
#include "ConcordTransformer.generated.h"

UCLASS(Abstract)
class CONCORD_API UConcordTransformer : public UConcordVertex
{
    GENERATED_BODY()
public:
    virtual FString GetCategory() const { return TEXT("Misc"); }
    virtual FConcordSharedExpression GetExpression(const FConcordMultiIndex& MultiIndex) const { checkNoEntry(); return MakeShared<const FConcordValueExpression<int32>>(0); }
    virtual FText GetTooltip() const override { return INVTEXT("A transformer that produces some outputs from its inputs."); }
};

UCLASS(Abstract)
class CONCORD_API UConcordAdaptiveTransformer : public UConcordTransformer
{
    GENERATED_BODY()
public:
    UConcordAdaptiveTransformer() : NumInputs(2) {}

    UPROPERTY()
    int32 NumInputs;
private:
    virtual FInputInfo GetSingleInputInfo() const { return { {}, {}, {}, "0"}; }
public:
    TArray<FInputInfo> GetInputInfo() const override
    {
        FInputInfo InputInfo = GetSingleInputInfo();
        TArray<FInputInfo> Ret; Ret.Init(InputInfo, NumInputs);
        return MoveTemp(Ret);
    }
};

// helpers
inline bool GetActualDimensionIndex(int32 DimensionIndex, const FConcordShape& ConnectedShape, int32& OutActualDimensionIndex, TOptional<FString>& OutErrorMessage)
{
    if (DimensionIndex >= ConnectedShape.Num() || -DimensionIndex > ConnectedShape.Num()) { OutErrorMessage = TEXT("Dimension index out of bounds."); return false; }
    OutActualDimensionIndex = (DimensionIndex >= 0) ? DimensionIndex : ConnectedShape.Num() + DimensionIndex; return true;
}

inline bool GetActualIndex(int32 Index, int32 Num, int32& OutActualIndex, TOptional<FString>& OutErrorMessage)
{
    if (Index >= Num || -Index > Num) { OutErrorMessage = TEXT("Index out of bounds."); return false; }
    OutActualIndex = (Index >= 0) ? Index : Num + Index; return true;
}
