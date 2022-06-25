// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Sampler/ConcordSampler.h"
#include "FactorGraph/ConcordFactorGraph.h"
#include "ConcordNativeModelImplementation.h"
#include "ConcordCrate.h"
#include "ConcordPattern.h"
#include "ConcordError.h"
#include "ConcordNativeModel.generated.h"

USTRUCT()
struct CONCORDCORE_API FConcordInputOutputInterfaceListener
{
    GENERATED_BODY()
#if WITH_EDITOR
    FConcordInputOutputInterfaceListener() : OutermostObject(nullptr) {}
    FConcordInputOutputInterfaceListener(UObject* InObject, const FName& InFunctionName);

    bool operator==(const FConcordInputOutputInterfaceListener& Other) const;
    UObject* GetListeningObject() const;
    const FName& GetFunctionName() const { return FunctionName; }
#endif
private:
#if WITH_EDITORONLY_DATA
    UPROPERTY()
    UObject* OutermostObject;

    UPROPERTY()
    FString SubobjectPath;

    UPROPERTY()
    FName FunctionName;
#endif
};

UCLASS(Abstract)
class CONCORDCORE_API UConcordLearnerFactoryBase : public UObject
{
    GENERATED_BODY()
public:
    bool IsEditorOnly() const override { return true; }

    virtual void BrowseDatasetDirectory() {}
    virtual void Learn() {}
    virtual void StopLearning() {}
};

UCLASS(CollapseCategories, BlueprintType)
class CONCORDCORE_API UConcordModelBase : public UObject
{
    GENERATED_BODY()
public:
    virtual TSharedPtr<const FConcordFactorGraph<float>> GetFactorGraph(EConcordCycleMode CycleMode, TOptional<FConcordError>& ErrorOut) const PURE_VIRTUAL(UConcordModelBase::GetFactorGraph, return {};)

    UPROPERTY(EditAnywhere, Instanced, NoClear, Category = "Concord Model", meta = (AllowAbstract = "false"))
    UConcordSamplerFactory* DefaultSamplerFactory;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Concord Model")
    TArray<UConcordCrate*> DefaultCrates;

    UPROPERTY()
    TArray<FName> OrderedParameterNames;

    UPROPERTY()
    TArray<FName> OrderedOutputNames;

#if WITH_EDITORONLY_DATA
    UPROPERTY(EditAnywhere, Instanced, NoClear, Category = "Preview")
    UConcordPattern* LatestPatternSampledFromEditor;

    UPROPERTY()
    TArray<FConcordInputOutputInterfaceListener> InputOutputInterfaceListeners;

    UPROPERTY(EditAnywhere, Instanced, Category = "Learning", meta = (AllowAbstract = "false"))
    UConcordLearnerFactoryBase* Learner;
private:
    UPROPERTY(Transient)
    class UConcordModelComponent* EditorModelComponent;
public:
#endif
#if WITH_EDITOR
    void PostEditImport() override;
    void PostDuplicate(bool bDuplicateForPIE) override;

    void SetLatestPatternFromSampledVariation(FConcordSampler* Sampler);
    void OnInputOutputInterfaceChanged();

    UFUNCTION(BlueprintCallable, Category = "Concord Model")
    class UConcordModelComponent* GetEditorModelComponent();

    UFUNCTION(CallInEditor, Category = "Learning")
    void BrowseDatasetDirectory();

    UFUNCTION(CallInEditor, Category = "Learning")
    void Learn();

    UFUNCTION(CallInEditor, Category = "Learning")
    void StopLearning();
#endif
};

USTRUCT()
struct FConcordNativeModelOutputInfo
{
    GENERATED_BODY()

    FConcordNativeModelOutputInfo() : Num(0), Type(EConcordValueType::Int) {}
    FConcordNativeModelOutputInfo(int32 InNum, EConcordValueType InType) : Num(InNum), Type(InType) {}

    UPROPERTY()
    int32 Num;

    UPROPERTY()
    EConcordValueType Type;
};

USTRUCT()
struct FConcordInstancedNativeModel
{
    GENERATED_BODY()

    FConcordInstancedNativeModel() : Model(nullptr) {}
    FConcordInstancedNativeModel(const FName& InName, const class UConcordNativeModel* InModel, const TArray<UConcordCrate*>& InInstanceCrates)
        : Name(InName), Model(InModel), InstanceCrates(InInstanceCrates)
    {}

    UPROPERTY()
    FName Name;

    UPROPERTY()
    const class UConcordNativeModel* Model;

    UPROPERTY()
    TArray<UConcordCrate*> InstanceCrates;
};

UCLASS()
class CONCORDCORE_API UConcordNativeModel : public UConcordModelBase
{
    GENERATED_BODY()
public:
#if WITH_EDITOR
    UFUNCTION(CallInEditor, Category = "Preview")
    void SamplePattern();
#endif

    void Init(FConcordFactorGraph<float>&& DynamicFactorGraph);
    TSharedPtr<const FConcordFactorGraph<float>> GetFactorGraph(EConcordCycleMode CycleMode, TOptional<FConcordError>& ErrorOut) const override;
#if WITH_EDITOR
    void ClearCachedFactorGraph() const;
#endif
    FString GetImplementationClassName() const { return ImplementationClassName; }
    FString GetImplementationClassPath() const { return ImplementationClassPath; }
    bool GetHasCycle() const { return bHasCycle; }
    bool HasInstancedModels() const { return !InstancedModels.IsEmpty(); }

    TTuple<EConcordValueType, const FConcordFactorGraphBlock*> GetParameterBlock(const FName& ParameterName) const;
    const FConcordNativeModelOutputInfo* GetOutputInfo(const FName& OutputName) const;
private:
    friend class FConcordModelNativization;

    UPROPERTY(VisibleAnywhere, Category = "Implementation")
    FString ImplementationClassName;

    UPROPERTY(VisibleAnywhere, Category = "Implementation")
    FString ImplementationClassPath;

    UPROPERTY()
    TArray<int32> RandomVariableStateCounts;

    UPROPERTY()
    TArray<int32> DisjointSubgraphRootFlatRandomVariableIndices;

    UPROPERTY()
    TMap<FName, FConcordFactorGraphBlock> VariationBlocks;

    UPROPERTY()
    TMap<FName, FConcordFactorGraphBlock> IntParameterBlocks;

    UPROPERTY()
    TMap<FName, FConcordFactorGraphBlock> FloatParameterBlocks;

    UPROPERTY()
    TArray<int32> IntParameterDefaultValues;

    UPROPERTY()
    TArray<float> FloatParameterDefaultValues;

#if WITH_EDITORONLY_DATA
    UPROPERTY()
    TArray<FName> TrainableFloatParameterBlockNames;
#endif

    UPROPERTY()
    TMap<FName, FConcordNativeModelOutputInfo> OutputInfos;

    UPROPERTY(VisibleAnywhere, Category = "Factor Graph")
    bool bHasCycle;

    UPROPERTY()
    TArray<FConcordInstancedNativeModel> InstancedModels;

    mutable TSharedPtr<FConcordFactorGraph<float>> FactorGraphFloat;

    UConcordNativeModelImplementation* GetImplementation() const;
    void CompleteFactorGraph() const;
};
