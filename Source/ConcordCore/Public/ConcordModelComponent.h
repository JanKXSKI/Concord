// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConcordValue.h"
#include "ConcordCrate.h"
#include "ConcordPattern.h"
#include "FactorGraph/ConcordFactorGraph.h"
#include "Components/ActorComponent.h"
#include "ConcordModelComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogConcordModelComponent, Log, All);

class UConcordModelBase;
class UConcordSamplerFactory;
class FConcordSampler;

UCLASS(meta=(BlueprintSpawnableComponent))
class CONCORDCORE_API UConcordModelComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UConcordModelComponent();

    UPROPERTY(EditAnywhere, Category = "Concord")
    UConcordModelBase* Model;

    UPROPERTY(EditAnywhere, Category = "Concord")
    TArray<UConcordCrate*> ComponentCrates;

    UPROPERTY(EditAnywhere, Instanced, NoClear, Category = "Concord", meta=(AllowAbstract="false"))
    UConcordSamplerFactory* SamplerFactoryOverride;

    UFUNCTION(BlueprintCallable, Category = "Concord")
    void SetIntParameter(FName ParameterName, int32 FlatParameterLocalIndex, int32 Value);

    UFUNCTION(BlueprintCallable, Category = "Concord")
    void SetIntParameterArray(FName ParameterName, const TArray<int32>& Array);

    UFUNCTION(BlueprintPure, Category = "Concord")
    int32 GetIntParameter(FName ParameterName, int32 FlatParameterLocalIndex) const;

    UFUNCTION(BlueprintCallable, Category = "Concord")
    void SetFloatParameter(FName ParameterName, int32 FlatParameterLocalIndex, float Value);

    UFUNCTION(BlueprintCallable, Category = "Concord")
    void SetFloatParameterArray(FName ParameterName, const TArray<float>& Array);

    UFUNCTION(BlueprintPure, Category = "Concord")
    float GetFloatParameter(FName ParameterName, int32 FlatParameterLocalIndex) const;

    UFUNCTION(BlueprintCallable, Category = "Concord")
    void ResetParameter(FName ParameterName);

    UFUNCTION(BlueprintCallable, Category = "Concord")
    void ObserveValue(FName BoxName, int32 FlatBoxLocalIndex, int32 Value);

    UFUNCTION(BlueprintCallable, Category = "Concord")
    void ObserveArray(FName BoxName, const TArray<int32>& Array);

    UFUNCTION(BlueprintCallable, Category = "Concord")
    void ObserveRandomVariable(FName BoxName, int32 FlatBoxLocalIndex);

    UFUNCTION(BlueprintCallable, Category = "Concord")
    void ObserveBox(FName BoxName);

    UFUNCTION(BlueprintCallable, Category = "Concord")
    void UnobserveRandomVariable(FName BoxName, int32 FlatBoxLocalIndex);

    UFUNCTION(BlueprintCallable, Category = "Concord")
    void UnobserveBox(FName BoxName);

    UFUNCTION(BlueprintCallable, Category = "Concord")
    void SetCrate(const FConcordCrateData& CrateData);

    UFUNCTION(BlueprintCallable, Category = "Concord")
    void SetNamedCrate(FName Name, const FConcordCrateData& CrateData);

    UFUNCTION(BlueprintCallable, Category = "Concord")
    void UnsetCrate(const FConcordCrateData& CrateData);

    DECLARE_DYNAMIC_DELEGATE_OneParam(FOnVariationSampled, float, Score);

    UFUNCTION(BlueprintCallable, Category = "Concord")
    void RunSamplerAsync(const FOnVariationSampled& OnVariationSampled);

    UFUNCTION(BlueprintCallable, Category = "Concord")
    void RunSamplerSync(float& Score);

    UFUNCTION(BlueprintCallable, Category = "Concord")
    void GetOutput(FName OutputName, TArray<int32>& Array);

    UFUNCTION(BlueprintCallable, Category = "Concord")
    void GetOutputOverwrite(FName OutputName, UPARAM(ref) TArray<int32>& InArray, TArray<int32>& Array);

    UFUNCTION(BlueprintCallable, Category = "Concord")
    void GetOutputOverwriteParameter(FName OutputName, UPARAM(ref) UConcordModelComponent*& ComponentWithParameter, FName ParameterName);

    UFUNCTION(BlueprintCallable, Category = "Concord")
    void OutputPattern(UConcordPattern*& Pattern);

    UFUNCTION(BlueprintCallable, Category = "Concord")
    void OverwritePattern(UPARAM(ref) UConcordPattern*& Pattern, UConcordPattern*& OutPattern);

    UFUNCTION(BlueprintCallable, Category = "Concord")
    void OutputPatternData(FConcordPatternData& PatternData);

    UFUNCTION(BlueprintCallable, Category = "Concord")
    void OutputCrateData(FConcordCrateData& CrateData);

    UFUNCTION(BlueprintCallable, Category = "Concord")
    void GetFloatOutput(FName OutputName, TArray<float>& Array);

    UFUNCTION(BlueprintCallable, Category = "Concord")
    void GetFloatOutputOverwrite(FName OutputName, UPARAM(ref) TArray<float>& InArray, TArray<float>& Array);

    void BeginPlay() override;
    void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

    DECLARE_DELEGATE_OneParam(FOnVariationSampledNative, float);
    FOnVariationSampledNative OnVariationSampledNative;

    bool CheckSamplerExists() const;
    bool CheckBoxExists(const FName& Name) const;
    bool CheckParameterExists(const FName& Name, EConcordValueType Type) const;
    bool CheckArrayNum(int32 BlockSize, int32 Num) const;
    bool CheckFlatLocalIndex(int32 BlockSize, int32 FlatBoxLocalIndex) const;
    bool CheckOutputExists(const FName& Name, EConcordValueType Type) const;
    TOptional<EConcordValueType> CheckParameterExists(const FName& Name, FConcordFactorGraphBlock* OutBlock) const;
    FConcordSampler* GetSampler() const { return Sampler.Get(); }
private:
    TSharedPtr<FConcordSampler> Sampler;
    FOnVariationSampled OnVariationSampled;
    bool bRerunOnDoneAsync;
    TMap<FName, TArray<FName>> NamedCrateBlockNames;

#if WITH_EDITOR
    friend class UConcordModelBase;
#endif
    void Setup();
    const UConcordSamplerFactory* GetActiveSamplerFactory() const;
};
