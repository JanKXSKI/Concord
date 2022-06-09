// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FactorGraph/ConcordFactorGraph.h"
#include "ConcordNativeModelImplementation.generated.h"

UCLASS()
class CONCORDCORE_API UConcordNativeModelImplementation : public UObject
{
    GENERATED_BODY()
public:
    virtual TArray<TUniquePtr<FConcordFactorHandleBase<float>>> GetFloatHandles() const PURE_VIRTUAL(UConcordNativeModelImplementation::GetFloatHandles, return {};);
    virtual FConcordFactorGraph<float>::FOutputs GetFloatOutputs() const PURE_VIRTUAL(UConcordNativeModelImplementation::GetFloatOutputs, return {};);
};

template<typename... FArgs>
struct FConcordIndexCount
{
    static constexpr int IndexCount = 0;
};

template<typename FArg, typename... FArgs>
struct FConcordIndexCount<FArg, FArgs...>
{
    static constexpr int IndexCount = FArg::IndexCount + FConcordIndexCount<FArgs...>::IndexCount;
};
