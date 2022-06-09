// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordNativeModel.h"
#include "EdGraph/EdGraph.h"
#include "ConcordModel.generated.h"

UCLASS()
class CONCORD_API UConcordModelGraphBase : public UEdGraph
{
    GENERATED_BODY()
public:
    bool IsEditorOnly() const override { return true; }
};

class UConcordVertex;
class UConcordBox;
class UConcordFactor;
class UConcordEmission;
class UConcordOutput;
class UConcordParameter;
class UConcordComposite;
class UConcordInstance;
class UConcordSamplerFactory;

UCLASS()
class CONCORD_API UConcordModel : public UConcordModelBase
{
    GENERATED_BODY()
public:
    UPROPERTY()
    TMap<FName, UConcordBox*> Boxes;

    UPROPERTY()
    TArray<UConcordFactor*> Factors;

    UPROPERTY()
    TArray<UConcordEmission*> Emissions;

    UPROPERTY()
    TMap<FName, UConcordOutput*> Outputs;

    UPROPERTY()
    TMap<FName, UConcordParameter*> Parameters;

    UPROPERTY()
    TMap<FName, UConcordComposite*> Composites;

    UPROPERTY()
    TMap<FName, UConcordInstance*> Instances;

    TSharedPtr<const FConcordFactorGraph<float>> GetFactorGraph(EConcordCycleMode CycleMode, TOptional<FConcordError>& ErrorOut) const override;

    bool IsNameTaken(const FName& Name) const;
    static FName SanitizeName(const FName& Name);

    TArray<UConcordVertex*> GetUpstreamSources() const;

#if WITH_EDITORONLY_DATA
    UPROPERTY()
    UConcordModelGraphBase* Graph;
#endif // WITH_EDITORONLY_DATA
};
