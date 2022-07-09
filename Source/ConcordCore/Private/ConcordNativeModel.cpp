// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "ConcordNativeModel.h"
#include "ConcordModelComponent.h"
#include "Sampler/ConcordGibbsSampler.h"
#include "Misc/MessageDialog.h"
#include "Misc/AsciiSet.h"

DEFINE_LOG_CATEGORY(LogConcordCore);

#if WITH_EDITOR
FConcordInputOutputInterfaceListener::FConcordInputOutputInterfaceListener(UObject* InObject, const FName& InFunctionName)
    : OutermostObject(InObject->GetOutermostObject())
    , SubobjectPath(InObject->GetPathName())
    , FunctionName(InFunctionName)
{
    constexpr FAsciiSet DotColon(".:");
    SubobjectPath = FAsciiSet::TrimPrefixWithout(SubobjectPath, DotColon);
    SubobjectPath.RightChopInline(1);
    SubobjectPath = FAsciiSet::TrimPrefixWithout(SubobjectPath, DotColon);
    if (!SubobjectPath.IsEmpty()) SubobjectPath.RightChopInline(1);
    SubobjectPath.Shrink();
}

bool FConcordInputOutputInterfaceListener::operator==(const FConcordInputOutputInterfaceListener& Other) const
{
    return OutermostObject == Other.OutermostObject &&
           SubobjectPath == Other.SubobjectPath &&
           FunctionName == Other.FunctionName;
}

UObject* FConcordInputOutputInterfaceListener::GetListeningObject() const
{
    constexpr FAsciiSet DotColon(".:");
    FStringView ObjectPathView = MakeStringView(SubobjectPath);
    UObject* Subobject = OutermostObject;
    while (!ObjectPathView.IsEmpty())
    {
        const FStringView InnerNameView = FAsciiSet::FindPrefixWithout(ObjectPathView, DotColon);
        Subobject = StaticFindObjectFast(nullptr, Subobject, FName(InnerNameView));
        ObjectPathView = FAsciiSet::TrimPrefixWithout(ObjectPathView, DotColon);
        if (ObjectPathView.IsEmpty()) break;
        ObjectPathView.RemovePrefix(1);
    }
    return Subobject;
}

void UConcordModelBase::PostEditImport()
{
    InputOutputInterfaceListeners.Empty();
}

void UConcordModelBase::PostDuplicate(bool bDuplicateForPIE)
{
    PostEditImport();
}

void UConcordModelBase::SetLatestPatternFromSampledVariation(FConcordSampler* Sampler)
{
    if (!LatestPatternSampledFromEditor) { Modify(); LatestPatternSampledFromEditor = NewObject<UConcordPattern>(this); }
    Sampler->SetColumnsFromOutputs(LatestPatternSampledFromEditor->PatternData);
}

void UConcordModelBase::OnInputOutputInterfaceChanged()
{
    for (int32 Index = 0; Index < InputOutputInterfaceListeners.Num(); ++Index)
    {
        const FConcordInputOutputInterfaceListener& Listener = InputOutputInterfaceListeners[Index];
        UObject* ListeningObject = Listener.GetListeningObject();
        UFunction* Function = ListeningObject ? ListeningObject->FindFunction(Listener.GetFunctionName()) : nullptr;
        if (!Function || Function->NumParms > 0)
        {
            Modify();
            InputOutputInterfaceListeners.RemoveAt(Index--);
        }
        else
        {
            ListeningObject->ProcessEvent(Function, nullptr);
        }
    }
}

UConcordModelComponent* UConcordModelBase::GetEditorModelComponent()
{
    if (!EditorModelComponent) EditorModelComponent = NewObject<UConcordModelComponent>();
    EditorModelComponent->Model = this;
    EditorModelComponent->Setup();
    return EditorModelComponent;
}

void UConcordModelBase::BrowseDatasetDirectory()
{
    if (!Learner)
    {
        FMessageDialog::Open(EAppMsgType::Ok, INVTEXT("Error: No learner set."));
        return;
    }
    Learner->BrowseDatasetDirectory();
}

void UConcordModelBase::Learn()
{
    if (!Learner)
    {
        FMessageDialog::Open(EAppMsgType::Ok, INVTEXT("Error: No learner set."));
        return;
    }
    Learner->Learn();
}

void UConcordModelBase::StopLearning()
{
    if (!Learner)
    {
        FMessageDialog::Open(EAppMsgType::Ok, INVTEXT("Error: No learner set."));
        return;
    }
    Learner->StopLearning();
}

void UConcordNativeModel::SamplePattern()
{
    UConcordModelComponent* ModelComponent = GetEditorModelComponent();
    if (!ModelComponent->CheckSamplerExists()) return;
    float Score; ModelComponent->RunSamplerSync(Score);
    SetLatestPatternFromSampledVariation(ModelComponent->GetSampler());
}
#endif // WITH_EDITOR

void UConcordNativeModel::Init(FConcordFactorGraph<float>&& DynamicFactorGraph)
{
    RandomVariableStateCounts = MoveTemp(DynamicFactorGraph.RandomVariableStateCounts);
    DisjointSubgraphRootFlatRandomVariableIndices = MoveTemp(DynamicFactorGraph.DisjointSubgraphRootFlatRandomVariableIndices);
    VariationBlocks = MoveTemp(DynamicFactorGraph.VariationBlocks);
    IntParameterBlocks = MoveTemp(DynamicFactorGraph.IntParameterBlocks);
    FloatParameterBlocks = MoveTemp(DynamicFactorGraph.FloatParameterBlocks);
    IntParameterDefaultValues = MoveTemp(DynamicFactorGraph.IntParameterDefaultValues);
    FloatParameterDefaultValues = MoveTemp(DynamicFactorGraph.FloatParameterDefaultValues);
#if WITH_EDITORONLY_DATA
    TrainableFloatParameterBlockNames = MoveTemp(DynamicFactorGraph.TrainableFloatParameterBlockNames);
#endif
    OutputInfos.Reset();
    for (const auto& NameOutputPair : DynamicFactorGraph.GetOutputs())
        OutputInfos.Add(NameOutputPair.Key, { NameOutputPair.Value->Num(), NameOutputPair.Value->GetType() });

    bHasCycle = DynamicFactorGraph.bHasCycle;

    InstancedModels.Reset();
}

TSharedPtr<const FConcordFactorGraph<float>> UConcordNativeModel::GetFactorGraph(EConcordCycleMode CycleMode, TOptional<FConcordError>& ErrorOut) const
{
    if (CycleMode != EConcordCycleMode::Ignore && bHasCycle)
    {
        ErrorOut = FConcordError { nullptr, TEXT("Requested a factor graph without cycles from a native model that was baked with a cyclic factor graph.") };
        return {};
    }

    if (FactorGraphFloat) return FactorGraphFloat;
    else FactorGraphFloat = MakeShared<FConcordFactorGraph<float>>();

    UConcordNativeModelImplementation* Implementation = GetImplementation();
    if (!Implementation)
    {
        ErrorOut = FConcordError { nullptr, FString::Printf(TEXT("Implementation for %s could not be found at %s"), *GetName(), *ImplementationClassPath) };
        return {};
    }
    FactorGraphFloat->Handles = Implementation->GetFloatHandles();
    FactorGraphFloat->Outputs = Implementation->GetFloatOutputs();
    CompleteFactorGraph();
    for (const FConcordInstancedNativeModel& InstancedModel : InstancedModels)
    {
        if (!InstancedModel.Model->DefaultSamplerFactory)
        {
            ErrorOut = FConcordError { nullptr, FString::Printf(TEXT("Instanced model needs to have its default sampler factory set.")) };
            return {};
        }
        TSharedPtr<const FConcordFactorGraph<float>> InstanceFactorGraph = InstancedModel.Model->GetFactorGraph(InstancedModel.Model->DefaultSamplerFactory->GetCycleMode(), ErrorOut);
        if (ErrorOut) return {};
        TOptional<FString> ErrorMessage;
        TSharedPtr<FConcordSampler> Sampler = InstancedModel.Model->DefaultSamplerFactory->CreateSampler(InstanceFactorGraph.ToSharedRef(), ErrorMessage);
        if (ErrorMessage)
        {
            ErrorOut = FConcordError { nullptr, TEXT("Error creating instance sampler: ") + ErrorMessage.GetValue() };
            return {};
        }
        for (const UConcordCrate* Crate : InstancedModel.Model->DefaultCrates) if (Crate) Sampler->GetEnvironment()->SetCrate(Crate->CrateData);
        for (const UConcordCrate* Crate : InstancedModel.InstanceCrates) if (Crate) Sampler->GetEnvironment()->SetCrate(Crate->CrateData);
        FactorGraphFloat->InstanceSamplers.Add({ InstancedModel.Name, Sampler.ToSharedRef() });
    }
    return FactorGraphFloat;
}

#if WITH_EDITOR
void UConcordNativeModel::ClearCachedFactorGraph() const
{
    FactorGraphFloat.Reset();
}
#endif

TTuple<EConcordValueType, const FConcordFactorGraphBlock*> UConcordNativeModel::GetParameterBlock(const FName& ParameterName) const
{
    const FConcordFactorGraphBlock* Block = IntParameterBlocks.Find(ParameterName);
    EConcordValueType Type = EConcordValueType::Int;
    if (!Block)
    {
        Block = FloatParameterBlocks.Find(ParameterName);
        Type = EConcordValueType::Float;
    }
    if (!Block) Type = EConcordValueType::Error;
    return MakeTuple(Type, Block);
}

const FConcordNativeModelOutputInfo* UConcordNativeModel::GetOutputInfo(const FName& OutputName) const
{
    return OutputInfos.Find(OutputName);
}

UConcordNativeModelImplementation* UConcordNativeModel::GetImplementation() const
{
    const FTopLevelAssetPath ClassAssetPath(*FString::Printf(TEXT("/Script/%s"), *ImplementationClassName), *ImplementationClassName);
    UClass* ImplementationClass = FindObject<UClass>(ClassAssetPath);
    if (ImplementationClass)
        return Cast<UConcordNativeModelImplementation>(ImplementationClass->GetDefaultObject());
    return nullptr;
}

void UConcordNativeModel::CompleteFactorGraph() const
{
    FactorGraphFloat->RandomVariableStateCounts = RandomVariableStateCounts;
    check(FactorGraphFloat->RandomVariableNeighboringHandles.IsEmpty());
    FactorGraphFloat->RandomVariableNeighboringHandles.SetNum(RandomVariableStateCounts.Num());
    for (const auto& Handle : FactorGraphFloat->Handles)
        for (int32 NeighboringFlatRandomVariableIndex : Handle->GetNeighboringFlatRandomVariableIndices())
            FactorGraphFloat->RandomVariableNeighboringHandles[NeighboringFlatRandomVariableIndex].Add(Handle.Get());
    FactorGraphFloat->DisjointSubgraphRootFlatRandomVariableIndices = DisjointSubgraphRootFlatRandomVariableIndices;
    FactorGraphFloat->VariationBlocks = VariationBlocks;
    FactorGraphFloat->IntParameterBlocks = IntParameterBlocks;
    FactorGraphFloat->FloatParameterBlocks = FloatParameterBlocks;
    FactorGraphFloat->IntParameterDefaultValues = IntParameterDefaultValues;
    FactorGraphFloat->FloatParameterDefaultValues = FloatParameterDefaultValues;
#if WITH_EDITORONLY_DATA
    FactorGraphFloat->TrainableFloatParameterBlockNames = TrainableFloatParameterBlockNames;
#endif
    FactorGraphFloat->bHasCycle = bHasCycle;
}
