// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "ConcordCompiler.h"
#include "ConcordModel.h"
#include "ConcordBox.h"
#include "ConcordComposite.h"
#include "ConcordInstance.h"
#include "ConcordParameter.h"
#include "ConcordFactor.h"
#include "ConcordEmission.h"
#include "ConcordOutput.h"
#include "ConcordFactorGraphDynamic.h"
#include "Transformers/ConcordTransformerGetDynamic.h"
#include "Transformers/ConcordTransformerTable.h"

DEFINE_LOG_CATEGORY(LogConcordCompiler);

using namespace ConcordFactorGraphDynamic;

FConcordCompiler::FResult::FResult()
    : FactorGraph(MakeShared<FConcordFactorGraph<float>>())
{}

FConcordCompiler::FConcordCompiler()
    : VariationBlockOffset(0)
    , IntParameterBlockOffset(0)
    , FloatParameterBlockOffset(0)
{}

FConcordCompiler::FResult FConcordCompiler::Compile(const UConcordModel* Model, EConcordCycleMode CycleMode)
{
    if (!Model->Emissions.IsEmpty() && CycleMode != EConcordCycleMode::Error)
    {
        UE_LOG(LogConcordCompiler, Warning, TEXT("Models with emission nodes cannot ignore or merge cycles in the factor graph, setting cycle mode to error."));
        CycleMode = EConcordCycleMode::Error;
    }

    FConcordCompiler Compiler; Compiler.ModelPath.Add(Model);
    FResult Result;
    if ((Result.Error = Compiler.CompileCompositeModel(TStrongObjectPtr<const UConcordModel>(Model), Result.FactorGraph.Get()))) return MoveTemp(Result);
    if ((Result.Error = Compiler.SetDisjointSubgraphRootFlatRandomVariableIndices(Result.FactorGraph.Get()))) return MoveTemp(Result);
    if ((Result.Error = Compiler.HandleCycles(CycleMode, Result.FactorGraph.Get()))) return MoveTemp(Result);
    Result.FactorGraph->CompactAndShrink();
    return MoveTemp(Result);
}

TOptional<FConcordError> FConcordCompiler::CompileCompositeModel(const TStrongObjectPtr<const UConcordModel>& CompositeModel, FConcordFactorGraph<float>& FactorGraph)
{
    // Setup boxes, composites, instances and parameters in order so that all upstream boxes, composites, instances and parameters have been setup
    TArray<UConcordVertex*> UpstreamSources = CompositeModel->GetUpstreamSources();
    TArray<TStrongObjectPtr<const UConcordModel>> CompositeModels; // ensure composite models are kept around until expressions have been created
    CompositeModels.Reserve(CompositeModel->Composites.Num());
    for (UConcordVertex* UpstreamSource : UpstreamSources)
    {
        if (UConcordBox* Box = Cast<UConcordBox>(UpstreamSource))
        {
            if (TOptional<FConcordError> Error = SetupBox(CompositeModel, *CompositeModel->Boxes.FindKey(Box), Box, FactorGraph))
                return Error.GetValue();
        }
        else if (UConcordComposite* Composite = Cast<UConcordComposite>(UpstreamSource))
        {
            if (TOptional<FConcordError> Error = SetupComposite(*CompositeModel->Composites.FindKey(Composite), Composite, CompositeModels, FactorGraph))
                return Error.GetValue();
        }
        else if (UConcordInstance* Instance = Cast<UConcordInstance>(UpstreamSource))
        {
            if (TOptional<FConcordError> Error = SetupInstance(*CompositeModel->Instances.FindKey(Instance), Instance, FactorGraph))
                return Error.GetValue();
        }
        else if (UConcordParameter* Parameter = Cast<UConcordParameter>(UpstreamSource))
        {
            if (TOptional<FConcordError> Error = SetupParameter(*CompositeModel->Parameters.FindKey(Parameter), Parameter, FactorGraph))
                return Error.GetValue();
        }
        else checkNoEntry();
    }

    // Setup factors and get expressions
    for (UConcordFactor* Factor : CompositeModel->Factors)
    {
        if (TOptional<FConcordError> Error = Factor->SetupGraph(VisitedVertices))
            return MakeCompositeError(Error.GetValue());
        check(Factor->GetType() == EConcordValueType::Float);

        ConcordShape::FShapeIterator It(Factor->GetShape());
        while (It.HasNext()) AddHandle(Factor->GetExpression(It.Next()), FactorGraph);
    }

    // Setup emissions
    for (UConcordEmission* Emission : CompositeModel->Emissions)
    {
        if (TOptional<FConcordError> Error = SetupEmission(CompositeModel, Emission, FactorGraph))
            return Error.GetValue();
    }

    // Setup outputs
    for (const TPair<FName, UConcordOutput*>& NameOutputPair : CompositeModel->Outputs)
    {
        UConcordOutput* Output = NameOutputPair.Value;
        if (TOptional<FConcordError> Error = Output->SetupGraph(VisitedVertices))
            return MakeCompositeError(Error.GetValue());

        // Only get output expressions for root model
        if (ModelPath.Num() == 1)
        {
            TArray<FConcordSharedExpression> ConnectedExpressions;
            ConcordShape::FShapeIterator It(Output->GetShape());
            while (It.HasNext())
                ConnectedExpressions.Add(Output->GetConnectedTransformers()[0]->GetExpression(It.Next()));
            FactorGraph.Outputs.Add(NameOutputPair.Key, MakeUnique<FOutput>(Output->GetType(), MoveTemp(ConnectedExpressions)));
        }
    }

    return {};
}

TOptional<FConcordError> FConcordCompiler::SetupBox(const TStrongObjectPtr<const UConcordModel>& CompositeModel, const FName& BoxName, UConcordBox* Box, FConcordFactorGraph<float>& FactorGraph)
{
    FName QualifiedBoxName = FName(CompositePrefix + BoxName.ToString());
    if (TOptional<FConcordError> Error = Box->SetupGraph(VisitedVertices))
        return MakeCompositeError(Error.GetValue()); // catches bad shape
    const int32 Size = ConcordShape::GetFlatNum(Box->GetShape());
    FactorGraph.VariationBlocks.Add(QualifiedBoxName, { VariationBlockOffset, Size });
    for (int32 FlatBoxLocalIndex = 0; FlatBoxLocalIndex < Size; ++FlatBoxLocalIndex)
    {
        const FConcordSharedRandomVariableExpression RandomVariableExpression = MakeShared<const FConcordRandomVariableExpression>(VariationBlockOffset + FlatBoxLocalIndex);
        FactorGraph.RandomVariableStateCounts.Add(Box->StateCount);
        FactorGraph.RandomVariableNeighboringHandles.AddDefaulted();
        AllRandomVariableExpressions.Add(RandomVariableExpression);
    }

    Box->ExpressionDelegate.BindRaw(this, &FConcordCompiler::GetRandomVariableExpression, Box->GetShape(), VariationBlockOffset);
    VariationBlockOffset += Size;

    return {};
}

TOptional<FConcordError> FConcordCompiler::SetupComposite(const FName& CompositeName, UConcordComposite* Composite, TArray<TStrongObjectPtr<const UConcordModel>>& OutCompositeModels, FConcordFactorGraph<float>& FactorGraph)
{
    if (!Composite->Model) return {};
    if (ModelPath.Contains(Composite->Model)) return MakeCompositeError({ Composite, TEXT("Composite cycle.") });

    check(!VisitedVertices.Contains(Composite));
    if (TOptional<FConcordError> Error = Composite->SetupGraph(VisitedVertices))
        return MakeCompositeError(Error.GetValue());

    TStrongObjectPtr<const UConcordModel>& CompositeModel = OutCompositeModels.AddDefaulted_GetRef();
    CompositeModel.Reset(DuplicateObject(Composite->Model, GetTransientPackage()));

    int32 CompositeInputIndex = 0;
    for (const FName& ParameterName : CompositeModel->OrderedParameterNames)
    {
        UConcordParameter* Parameter = CompositeModel->Parameters[ParameterName];
        if (Parameter->bLocal) continue;
        UConcordTransformer* ConnectedTransformer = Composite->GetConnectedTransformers()[CompositeInputIndex++];
        if (ConnectedTransformer->IsPinDefaultValue()) continue;
        // manual vertex setup
        Parameter->Shape = ConnectedTransformer->GetShape();
        Parameter->Type = Parameter->GetParameterType();
        Parameter->ExpressionDelegate.BindUObject(ConnectedTransformer, &UConcordTransformer::GetExpression);
        VisitedVertices.Add(Parameter);
    }

    ModelPath.Push(Composite->Model);
    const FString CompositePrefixExtension = CompositeName.ToString() + TEXT(".");
    CompositePrefix += CompositePrefixExtension;
    if (TOptional<FConcordError> Error = CompileCompositeModel(CompositeModel, FactorGraph))
        return Error.GetValue();
    ModelPath.Pop(false);
    CompositePrefix.LeftChopInline(CompositePrefixExtension.Len(), false);

    for (const TPair<FName, UConcordOutput*>& NameOutputPair : CompositeModel->Outputs)
    {
        UConcordCompositeOutput* CompositeOutput = Composite->GetOutputTransformer(NameOutputPair.Key);
        // manual vertex setup
        CompositeOutput->Shape = NameOutputPair.Value->GetShape();
        CompositeOutput->Type = NameOutputPair.Value->GetType();
        CompositeOutput->ExpressionDelegate.BindUObject(NameOutputPair.Value->GetConnectedTransformers()[0], &UConcordTransformer::GetExpression);
        VisitedVertices.Add(CompositeOutput);
    }

    return {};
}

TOptional<FConcordError> FConcordCompiler::SetupInstance(const FName& InstanceName, UConcordInstance* Instance, FConcordFactorGraph<float>& FactorGraph)
{
    if (!Instance->Model) return {};
    if (!Instance->Model->DefaultSamplerFactory) return MakeCompositeError({ Instance, TEXT("A model that is instanced needs to have its default sampler factory set.") });

    check(!VisitedVertices.Contains(Instance));
    if (TOptional<FConcordError> Error = Instance->SetupGraph(VisitedVertices))
        return MakeCompositeError(Error.GetValue());

    const FString InstancePrefixExtension = InstanceName.ToString() + TEXT(".");
    TSharedPtr<const FConcordFactorGraph<float>> InstanceFactorGraph;
    if (const UConcordNativeModel* NativeModel = Cast<UConcordNativeModel>(Instance->Model))
    {
        TOptional<FConcordError> Error;
        InstanceFactorGraph = NativeModel->GetFactorGraph(NativeModel->DefaultSamplerFactory->GetCycleMode(), Error);
        if (Error) return MakeCompositeError(MoveTemp(Error.GetValue()));
    }
    else if (const UConcordModel* Model = Cast<UConcordModel>(Instance->Model))
    {
        if (ModelPath.Contains(Model)) return MakeCompositeError({ Instance, TEXT("Instance cycle.") });
        if (CompiledModels.Contains(Model))
        {
            InstanceFactorGraph = CompiledModels[Model];
        }
        else
        {
            FResult CompileResult = Compile(Model, Model->DefaultSamplerFactory->GetCycleMode());
            if (CompileResult.Error) return MakeCompositeError(FConcordError { nullptr, FString::Printf(TEXT("Failed to compile instance of model %s: "), *Model->GetName()) + CompileResult.Error.GetValue().Message });
            CompiledModels.Add(Model, CompileResult.FactorGraph);
            InstanceFactorGraph = CompileResult.FactorGraph;
            Instance->UpdateCachedInputOutputInfo();
        }
    }
    else { checkNoEntry(); return {}; }
    TOptional<FString> ErrorMessage;
    TSharedPtr<FConcordSampler> Sampler = Instance->Model->DefaultSamplerFactory->CreateSampler(InstanceFactorGraph.ToSharedRef(), ErrorMessage);
    if (ErrorMessage) return MakeCompositeError({ Instance, TEXT("Error creating instance sampler: ") + ErrorMessage.GetValue() });
    for (const UConcordCrate* Crate : Instance->Model->DefaultCrates) if (Crate) Sampler->GetEnvironment()->SetCrate(Crate->CrateData);
    for (const UConcordCrate* Crate : Instance->InstanceCrates) if (Crate) Sampler->GetEnvironment()->SetCrate(Crate->CrateData);
    FactorGraph.InstanceSamplers.Add({ InstanceName, Sampler.ToSharedRef() });

    int32 InstanceInputIndex = 0;
    for (const FName& ParameterName : Instance->Model->OrderedParameterNames)
    {
        const auto* BlockPtr = InstanceFactorGraph->GetParameterBlocks<int32>().Find(ParameterName);
        if (!BlockPtr) BlockPtr = InstanceFactorGraph->GetParameterBlocks<float>().Find(ParameterName);
        if (BlockPtr->bLocal) continue;
        UConcordTransformer* ConnectedTransformer = Instance->GetConnectedTransformers()[InstanceInputIndex++];
        if (ConnectedTransformer->IsPinDefaultValue()) continue;

        TArray<FConcordSharedExpression> ConnectedExpressions;
        ConcordShape::FShapeIterator It(ConnectedTransformer->GetShape());
        TArray<int32> NeighboringFlatRandomVariableIndices;
        while (It.HasNext())
        {
            FConcordSharedExpression Expression = ConnectedTransformer->GetExpression(It.Next());
            Expression->AddNeighboringFlatRandomVariableIndices(NeighboringFlatRandomVariableIndices);
            if (!NeighboringFlatRandomVariableIndices.IsEmpty()) return MakeCompositeError({ Instance, TEXT("An instance cannot depend on random variables of the instancing model.") });
            ConnectedExpressions.Add(MoveTemp(Expression));
        }
        if (ConnectedExpressions.Num() != BlockPtr->Size)
            return MakeCompositeError({ Instance, FString::Printf(TEXT("Flat number of expressions connected to %s does not match the expected size %i."), *ParameterName.ToString(), BlockPtr->Size) });
        const FName OutputName = FName(InstancePrefixExtension + ParameterName.ToString() + TEXT(".Source"));
        FactorGraph.Outputs.Add(OutputName, MakeUnique<FOutput>(ConnectedTransformer->GetType(), MoveTemp(ConnectedExpressions)));
    }

    for (const auto& NameOutputPair : InstanceFactorGraph->GetOutputs())
    {
        UConcordInstanceOutput* InstanceOutput = Instance->GetOutputTransformer(NameOutputPair.Key);
        // manual vertex setup
        InstanceOutput->Shape = { NameOutputPair.Value->Num() };
        InstanceOutput->Type = NameOutputPair.Value->GetType();
        const FName ParameterName = FName(InstancePrefixExtension + NameOutputPair.Key.ToString() + TEXT(".Target"));
        switch (InstanceOutput->Type)
        {
        case EConcordValueType::Int: AddTargetParameter<int32>(ParameterName, InstanceOutput, FactorGraph); break;
        case EConcordValueType::Float: AddTargetParameter<float>(ParameterName, InstanceOutput, FactorGraph); break;
        default: checkNoEntry();
        }
        VisitedVertices.Add(InstanceOutput);
    }

    return {};
}

TOptional<FConcordError> FConcordCompiler::SetupParameter(const FName& ParameterName, UConcordParameter* Parameter, FConcordFactorGraph<float>& FactorGraph)
{
    if (VisitedVertices.Contains(Parameter)) return {}; // overridden parameter vertices have been manually setup

    if (TOptional<FConcordError> Error = Parameter->SetupGraph(VisitedVertices))
        return MakeCompositeError(Error.GetValue()); // catches bad shape
    FName QualifiedParameterName = FName(CompositePrefix + ParameterName.ToString());
    switch (Parameter->GetType())
    {
    case EConcordValueType::Int: AddParameter<int32>(QualifiedParameterName, Parameter, FactorGraph); break;
    case EConcordValueType::Float: AddParameter<float>(QualifiedParameterName, Parameter, FactorGraph); break;
    default: checkNoEntry();
    }

    return {};
}

TOptional<FConcordError> FConcordCompiler::SetupEmission(const TStrongObjectPtr<const UConcordModel>& CompositeModel, UConcordEmission* Emission, FConcordFactorGraph<float>& FactorGraph)
{
    if (TOptional<FConcordError> Error = Emission->SetupGraph(VisitedVertices))
        return MakeCompositeError(Error.GetValue()); // catches bad connections
    UConcordBox* HiddenBox = Emission->GetConnectedBox(0);
    const FName HiddenBoxName = *CompositeModel->Boxes.FindKey(HiddenBox);
    const FName QualifiedHiddenBoxName = FName(CompositePrefix + HiddenBoxName.ToString());
    const int32 HiddenBoxOffset = FactorGraph.VariationBlocks[QualifiedHiddenBoxName].Offset;
    UConcordBox* ObservedBox = Emission->GetConnectedBox(1);
    const FName ObservedBoxName = *CompositeModel->Boxes.FindKey(ObservedBox);
    const FName QualifiedObservedBoxName = FName(CompositePrefix + ObservedBoxName.ToString());
    const int32 ObservedBoxOffset = FactorGraph.VariationBlocks[QualifiedObservedBoxName].Offset;

    const FName InitialName = FName(QualifiedHiddenBoxName.ToString() + TEXT(".Initial"));
    if (!FactorGraph.GetParameterBlocks<float>().Contains(InitialName))
    {
        TArray<FConcordSharedExpression> InitialSourceExpressions; InitialSourceExpressions.Reserve(2 + HiddenBox->StateCount);
        InitialSourceExpressions.Add(AllRandomVariableExpressions[HiddenBoxOffset]);
        InitialSourceExpressions.Add(MakeShared<const FConcordValueExpression<float>>(0));
        AddEmissionParameter(InitialName, HiddenBox->StateCount, FactorGraph, InitialSourceExpressions);
        AddHandle(MakeShared<const FConcordGetExpression>(MoveTemp(InitialSourceExpressions)), FactorGraph);
    }

    const FName TransitionName = FName(QualifiedHiddenBoxName.ToString() + TEXT(".Transition"));
    if (!FactorGraph.GetParameterBlocks<float>().Contains(TransitionName))
    {
        TArray<FConcordSharedExpression> TransitionSourceExpressions; TransitionSourceExpressions.Reserve(2 + HiddenBox->StateCount * HiddenBox->StateCount);
        TransitionSourceExpressions.Add(AllRandomVariableExpressions[0]); // replaced in chain
        TransitionSourceExpressions.Add(AllRandomVariableExpressions[0]); // replaced in chain
        AddEmissionParameter(TransitionName, HiddenBox->StateCount * HiddenBox->StateCount, FactorGraph, TransitionSourceExpressions);
        for (int32 FlatBoxLocalIndex = 0; FlatBoxLocalIndex < ConcordShape::GetFlatNum(HiddenBox->GetShape()) - 1; ++FlatBoxLocalIndex)
        {
            TArray<FConcordSharedExpression> SourceExpressions = TransitionSourceExpressions;
            SourceExpressions[0] = AllRandomVariableExpressions[HiddenBoxOffset + FlatBoxLocalIndex];
            SourceExpressions[1] = AllRandomVariableExpressions[HiddenBoxOffset + FlatBoxLocalIndex + 1];
            AddHandle(MakeShared<const FConcordTransformerTableExpression>(MoveTemp(SourceExpressions), FConcordShape { HiddenBox->StateCount, HiddenBox->StateCount }), FactorGraph);
        }
    }

    int32 Stride = 1; 
    for (int32 ObservedDimIndex = ObservedBox->GetShape().Num() - 1; ObservedDimIndex >= HiddenBox->GetShape().Num(); --ObservedDimIndex)
        Stride *= ObservedBox->GetShape()[ObservedDimIndex];
    FString EmissionNameBase = QualifiedHiddenBoxName.ToString() + TEXT(".") + ObservedBoxName.ToString() + TEXT(".");
    for (int32 Offset = 0; Offset < Stride; ++Offset)
    {
        const FName EmissionName = FName(EmissionNameBase + FString::FromInt(Offset) + TEXT(".") + FString::FromInt(Stride) + TEXT(".Emission"));
        if (!FactorGraph.GetParameterBlocks<float>().Contains(EmissionName))
        {
            TArray<FConcordSharedExpression> EmissionSourceExpressions; EmissionSourceExpressions.Reserve(2 + HiddenBox->StateCount * ObservedBox->StateCount);
            EmissionSourceExpressions.Add(AllRandomVariableExpressions[0]); // replaced in chain
            EmissionSourceExpressions.Add(AllRandomVariableExpressions[0]); // replaced in chain
            AddEmissionParameter(EmissionName, HiddenBox->StateCount * ObservedBox->StateCount, FactorGraph, EmissionSourceExpressions);
            check(ConcordShape::GetFlatNum(HiddenBox->GetShape()) == ConcordShape::GetFlatNum(ObservedBox->GetShape()) / Stride);
            for (int32 FlatBoxLocalIndex = 0; FlatBoxLocalIndex < ConcordShape::GetFlatNum(HiddenBox->GetShape()); ++FlatBoxLocalIndex)
            {
                TArray<FConcordSharedExpression> SourceExpressions = EmissionSourceExpressions;
                SourceExpressions[0] = AllRandomVariableExpressions[HiddenBoxOffset + FlatBoxLocalIndex];
                SourceExpressions[1] = AllRandomVariableExpressions[ObservedBoxOffset + Offset + FlatBoxLocalIndex * Stride];
                AddHandle(MakeShared<const FConcordTransformerTableExpression>(MoveTemp(SourceExpressions), FConcordShape { HiddenBox->StateCount, ObservedBox->StateCount }), FactorGraph);
            }
        }
    }

    return {};
}

template<typename FValue>
void FConcordCompiler::AddParameter(const FName& ParameterName, UConcordParameter* Parameter, FConcordFactorGraph<float>& FactorGraph)
{
    const int32 Size = ConcordShape::GetFlatNum(Parameter->GetShape());
    FactorGraph.GetParameterBlocks<FValue>().Add(ParameterName, { GetParameterBlockOffset<FValue>(), Size, Parameter->bLocal });
    for (int32 FlatParameterLocalIndex = 0; FlatParameterLocalIndex < Size; ++FlatParameterLocalIndex)
    {
        FactorGraph.GetParameterDefaultValues<FValue>().Add(Parameter->GetDefaultValue(FlatParameterLocalIndex).Get<FValue>());
        GetAllParameterExpressions<FValue>().Add(MakeShared<const FConcordParameterExpression<FValue>>(GetParameterBlockOffset<FValue>() + FlatParameterLocalIndex));
    }
    Parameter->ExpressionDelegate.BindRaw(this, &FConcordCompiler::GetParameterExpression<FValue>, Parameter->GetShape(), GetParameterBlockOffset<FValue>());
    GetParameterBlockOffset<FValue>() += Size;
}

template<typename FValue>
void FConcordCompiler::AddTargetParameter(const FName& TargetParameterName, UConcordInstanceOutput* ObservedInstanceOutput, FConcordFactorGraph<float>& FactorGraph)
{
    const int32 Size = ConcordShape::GetFlatNum(ObservedInstanceOutput->GetShape());
    FactorGraph.GetParameterBlocks<FValue>().Add(TargetParameterName, { GetParameterBlockOffset<FValue>(), Size });
    for (int32 FlatParameterLocalIndex = 0; FlatParameterLocalIndex < Size; ++FlatParameterLocalIndex)
    {
        FactorGraph.GetParameterDefaultValues<FValue>().Add(0);
        GetAllParameterExpressions<FValue>().Add(MakeShared<const FConcordParameterExpression<FValue>>(GetParameterBlockOffset<FValue>() + FlatParameterLocalIndex));
    }
    ObservedInstanceOutput->ExpressionDelegate.BindRaw(this, &FConcordCompiler::GetParameterExpression<FValue>, ObservedInstanceOutput->GetShape(), GetParameterBlockOffset<FValue>());
    GetParameterBlockOffset<FValue>() += Size;
}

void FConcordCompiler::AddEmissionParameter(const FName& EmissionParameterName, int32 Size, FConcordFactorGraph<float>& FactorGraph, TArray<FConcordSharedExpression>& OutParameterExpressions)
{
    FactorGraph.GetParameterBlocks<float>().Add(EmissionParameterName, { GetParameterBlockOffset<float>(), Size });
    for (int32 FlatParameterLocalIndex = 0; FlatParameterLocalIndex < Size; ++FlatParameterLocalIndex)
    {
        FactorGraph.GetParameterDefaultValues<float>().Add(0);
        GetAllParameterExpressions<float>().Add(MakeShared<const FConcordParameterExpression<float>>(GetParameterBlockOffset<float>() + FlatParameterLocalIndex));
        OutParameterExpressions.Add(GetAllParameterExpressions<float>().Last());
    }
    GetParameterBlockOffset<float>() += Size;
#if WITH_EDITORONLY_DATA
    FactorGraph.TrainableFloatParameterBlockNames.Add(EmissionParameterName);
#endif
}

void FConcordCompiler::AddHandle(const FConcordSharedExpression& FactorExpression, FConcordFactorGraph<float>& FactorGraph) const
{
    FactorGraph.Handles.Add(MakeUnique<FAtomicHandle>(FactorExpression));
    for (int32 FlatRandomVariableIndex : FactorGraph.Handles.Last()->GetNeighboringFlatRandomVariableIndices())
        FactorGraph.RandomVariableNeighboringHandles[FlatRandomVariableIndex].AddUnique(FactorGraph.Handles.Last().Get());
}

FConcordError FConcordCompiler::MakeCompositeError(const FConcordError& Error) const
{
    if (CompositePrefix.IsEmpty()) return Error;
    FString CompositeName = CompositePrefix;
    int32 DotIndex; if (CompositeName.FindChar('.', DotIndex)) CompositeName.LeftInline(DotIndex, false);
    FString ErrorVertexClassName = TEXT("");
    if (Error.Vertex) ErrorVertexClassName = Error.Vertex->GetClass()->GetName() + TEXT(": ");
    return { ModelPath[0]->Composites[FName(CompositeName)], FString::Printf(TEXT("%s%s (Composite path so far: %s)"), *ErrorVertexClassName, *Error.Message, *CompositePrefix.LeftChop(1)) };
}

TOptional<FConcordError> FConcordCompiler::SetDisjointSubgraphRootFlatRandomVariableIndices(FConcordFactorGraph<float>& FactorGraph) const
{
    TSet<int32> AddedFlatRandomVariableIndices;
    while (AddedFlatRandomVariableIndices.Num() != AllRandomVariableExpressions.Num())
    {
        for (const FConcordSharedRandomVariableExpression& Expression : AllRandomVariableExpressions)
            if (!AddedFlatRandomVariableIndices.Contains(Expression->FlatIndex))
            {
                FactorGraph.DisjointSubgraphRootFlatRandomVariableIndices.Add(Expression->FlatIndex);
                break;
            }
        AddConnected(FactorGraph.DisjointSubgraphRootFlatRandomVariableIndices.Last(), AddedFlatRandomVariableIndices, FactorGraph);
    }
    return {};
}

void FConcordCompiler::AddConnected(int32 FlatRandomVariableIndex, TSet<int32>& AddedFlatRandomVariableIndices, const FConcordFactorGraph<float>& FactorGraph) const
{
    AddedFlatRandomVariableIndices.Add(FlatRandomVariableIndex);
    for (const auto* NeighboringHandle : FactorGraph.GetNeighboringHandles(FlatRandomVariableIndex))
        for (int32 NeighboringIndex : NeighboringHandle->GetNeighboringFlatRandomVariableIndices())
            if (!AddedFlatRandomVariableIndices.Contains(NeighboringIndex))
                AddConnected(NeighboringIndex, AddedFlatRandomVariableIndices, FactorGraph);
}

TOptional<FConcordError> FConcordCompiler::HandleCycles(EConcordCycleMode CycleMode, FConcordFactorGraph<float>& FactorGraph) const
{
    TArray<int32> RandomVariablePath;
    TArray<const FConcordHandle*> FactorPath;
    for (int32 RootFlatRandomVariableIndex : FactorGraph.DisjointSubgraphRootFlatRandomVariableIndices)
    {
        bool bSubgraphHasCycles = true;
        while (bSubgraphHasCycles)
        {
            bSubgraphHasCycles = false;
            for (const auto* Handle : FactorGraph.GetNeighboringHandles(RootFlatRandomVariableIndex))
                if (MergeCycle(RootFlatRandomVariableIndex, Handle, RandomVariablePath, FactorPath, FactorGraph))
                {
                    if (CycleMode == EConcordCycleMode::Error) return GetCycleError(RandomVariablePath, FactorGraph);
                    if (CycleMode == EConcordCycleMode::Ignore) { FactorGraph.bHasCycle = true; return {}; }
                    bSubgraphHasCycles = true;
                    RandomVariablePath.Reset();
                    FactorPath.Reset();
                    break;
                }
        }
    }
    return {};
}

bool FConcordCompiler::MergeCycle(int32 FlatRandomVariableIndex, const FConcordHandle* Handle, TArray<int32>& RandomVariablePath, TArray<const FConcordHandle*>& FactorPath, FConcordFactorGraph<float>& FactorGraph) const
{
    int32 PathIndex = RandomVariablePath.FindLast(FlatRandomVariableIndex);
    if (PathIndex == INDEX_NONE)
    {
        RandomVariablePath.Add(FlatRandomVariableIndex);
        FactorPath.Add(Handle);
        for (int32 NeighboringIndex : Handle->GetNeighboringFlatRandomVariableIndices())
            if (NeighboringIndex != FlatRandomVariableIndex)
                for (const auto* NeighboringHandle : FactorGraph.GetNeighboringHandles(NeighboringIndex))
                    if (NeighboringHandle != Handle)
                        if (MergeCycle(NeighboringIndex, NeighboringHandle, RandomVariablePath, FactorPath, FactorGraph))
                            return true;
        RandomVariablePath.Pop();
        FactorPath.Pop();
        return false;
    }
    RandomVariablePath.RemoveAtSwap(0, PathIndex, false);
    FactorPath.RemoveAtSwap(0, PathIndex, false);
    TUniquePtr<FMergedHandle> MergedHandle = MakeUnique<FMergedHandle>();
    TArray<const FConcordHandle*> HandlesToMerge = FactorPath;
    for (int OwnedHandleIndex = 0; OwnedHandleIndex < FactorGraph.Handles.Num() && !HandlesToMerge.IsEmpty(); ++OwnedHandleIndex)
        for (const auto* HandleToMerge : HandlesToMerge)
        {
            if (FactorGraph.Handles[OwnedHandleIndex].Get() != HandleToMerge) continue;
            if (FMergedHandle* MergedHandleToMerge = static_cast<FHandle*>(FactorGraph.Handles[OwnedHandleIndex].Get())->GetMergedHandle())
                for (TUniquePtr<FAtomicHandle>& ChildHandle : MergedHandleToMerge->Children)
                    MergedHandle->AddHandle(MoveTemp(ChildHandle));
            else // HandleToMerge is not a merged handle, assume it is an atomic handle
                MergedHandle->AddHandle(TUniquePtr<FAtomicHandle>(static_cast<FAtomicHandle*>(FactorGraph.Handles[OwnedHandleIndex].Release())));
            HandlesToMerge.RemoveSingleSwap(HandleToMerge, false);
            FactorGraph.Handles.RemoveAtSwap(OwnedHandleIndex--, 1, false);
            break;
        }
    for (int32 AffectedIndex : MergedHandle->GetNeighboringFlatRandomVariableIndices())
    {
        FactorGraph.RandomVariableNeighboringHandles[AffectedIndex].RemoveAllSwap([&](const FConcordHandle* InHandle){ return FactorPath.Find(InHandle) != INDEX_NONE; }, false);
        FactorGraph.RandomVariableNeighboringHandles[AffectedIndex].Add(MergedHandle.Get());
    }
    UE_LOG(LogConcordCompiler, Log, TEXT("Merged cycle with %i neighboring random variables."), MergedHandle->GetNeighboringFlatRandomVariableIndices().Num());
    FactorGraph.Handles.Add(MoveTemp(MergedHandle));
    return true;
}

FConcordError FConcordCompiler::GetCycleError(const TArray<int32>& RandomVariableCycle, FConcordFactorGraph<float>& FactorGraph)
{
    check(!RandomVariableCycle.IsEmpty());
    FString ErrorMessage = FString::Printf(TEXT("Cycle detected (cycle mode is set to error): %s"), *GetRandomVariableIndexString(RandomVariableCycle[0], FactorGraph));
    for (int32 Index = 1; Index < FMath::Min(5, RandomVariableCycle.Num()); ++Index) ErrorMessage += FString::Printf(TEXT(", %s"), *GetRandomVariableIndexString(RandomVariableCycle[Index], FactorGraph));
    if (RandomVariableCycle.Num() > 5) ErrorMessage += TEXT(", ...");
    return { nullptr, ErrorMessage };
}

FString FConcordCompiler::GetRandomVariableIndexString(int32 FlatRandomVariableIndex, FConcordFactorGraph<float>& FactorGraph)
{
    for (const TPair<FName, FConcordFactorGraphBlock>& NameVariationBlockPair : FactorGraph.VariationBlocks)
    {
        const int32 FlatBoxLocalIndex = FlatRandomVariableIndex - NameVariationBlockPair.Value.Offset;
        if (FlatBoxLocalIndex >= NameVariationBlockPair.Value.Size) continue;
        return FString::Printf(TEXT("%s[%i]"), *NameVariationBlockPair.Key.ToString(), FlatBoxLocalIndex);
    }
    checkNoEntry();
    return {};
}

FConcordSharedExpression FConcordCompiler::GetRandomVariableExpression(const FConcordMultiIndex& BoxLocalIndex, FConcordShape BoxShape, int32 BlockOffset) const
{
    return AllRandomVariableExpressions[BlockOffset + ConcordShape::FlattenIndex(BoxLocalIndex, BoxShape)];
}

template<typename FValue>
FConcordSharedExpression FConcordCompiler::GetParameterExpression(const FConcordMultiIndex& ParameterLocalIndex, FConcordShape ParameterShape, int32 BlockOffset) const
{
    return GetAllParameterExpressions<FValue>()[BlockOffset + ConcordShape::FlattenIndex(ParameterLocalIndex, ParameterShape)];
}
