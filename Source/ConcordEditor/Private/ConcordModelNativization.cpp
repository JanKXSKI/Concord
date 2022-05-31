// Copyright Jan Klimaschewski. All Rights Reserved.

#include "ConcordModelNativization.h"
#include "ConcordInstance.h"
#include "AssetToolsModule.h"
#include "DesktopPlatformModule.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Misc/App.h"
#include "Interfaces/IPluginManager.h"
#include "Internationalization/Regex.h"
#include "Algo/MaxElement.h"
#include "Misc/MessageDialog.h"
#if WITH_LIVE_CODING
#include "ILiveCodingModule.h"
#endif

using namespace ConcordFactorGraphDynamic;
using namespace Concord;

UConcordNativeModel* FConcordModelNativization::CreateNativeModel(const UConcordModel* Model)
{
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (!DesktopPlatform) return nullptr;
    TArray<FString> FileNames;
    FString DefaultPath = FPaths::Combine(IPluginManager::Get().FindPlugin("Concord")->GetBaseDir(), "Source", "ConcordCore", "Private", "Nativization");
    if (!IFileManager::Get().DirectoryExists(*DefaultPath)) IFileManager::Get().MakeDirectory(*DefaultPath);
    if (!DesktopPlatform->SaveFileDialog(FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
                                         TEXT("Native Concord Model Implementation Directory"),
                                         DefaultPath,
                                         Model->GetName() + TEXT("ConcordNativeImplementation.h"),
                                         TEXT("Cpp Header File|*.h"),
                                         EFileDialogFlags::None,
                                         FileNames)) return nullptr;
    if (FileNames.IsEmpty() || !FPaths::MakePathRelativeTo(FileNames[0], *FPaths::ProjectDir())) return nullptr;
    FString AssetName = Model->GetName() + TEXT("Native");
    FString PackagePath = FPaths::GetPath(Model->GetPackage()->GetName());
    UConcordNativeModel* NativeModel = Cast<UConcordNativeModel>(FAssetToolsModule::GetModule().Get().CreateAsset(AssetName, PackagePath, UConcordNativeModel::StaticClass(), nullptr));
    if (!NativeModel) return nullptr;
    NativeModel->ImplementationClassPath = FileNames[0];
    NativeModel->ImplementationClassName = FPaths::GetBaseFilename(FileNames[0]);
    return NativeModel;
}

TOptional<FConcordError> FConcordModelNativization::SetupNativeModel(UConcordNativeModel* NativeModel, const UConcordModel* Model, EConcordCycleMode CycleMode, bool bLiveCoding)
{
    if (!NativeModel) return FConcordError { nullptr, TEXT("Native model asset was not successfully created.") };
    FConcordCompiler::FResult CompilerResult = FConcordCompiler::Compile(Model, CycleMode);
    if (CompilerResult.Error) return CompilerResult.Error;

    NativeModel->Modify();
    NativeModel->DefaultSamplerFactory = DuplicateObject(Model->DefaultSamplerFactory, NativeModel);
    NativeModel->DefaultCrates = Model->DefaultCrates;
    NativeModel->OrderedParameterNames = Model->OrderedParameterNames;
    NativeModel->OrderedOutputNames = Model->OrderedOutputNames;

    FString FullImplementationClassPath = FPaths::ProjectDir() / NativeModel->ImplementationClassPath;
    if (!FFileHelper::SaveStringToFile(GetImplementationCode(NativeModel->ImplementationClassName, CompilerResult.FactorGraph.Get()), *FullImplementationClassPath))
        return FConcordError { nullptr, FString::Printf(TEXT("Could not write implementation code to selected path: %s."), *FullImplementationClassPath) };

    NativeModel->Init(MoveTemp(CompilerResult.FactorGraph.Get()));
    for (const auto& NameSamplerPair : CompilerResult.FactorGraph->GetInstanceSamplers())
    {
        const UConcordInstance* Instance = Model->Instances[NameSamplerPair.Key];
        const UConcordNativeModel* InstancedNativeModel = Cast<UConcordNativeModel>(Instance->Model);
        if (!InstancedNativeModel) return FConcordError { Instance, TEXT("Instanced model is not native.") };
        NativeModel->InstancedModels.Add({ NameSamplerPair.Key, InstancedNativeModel, Instance->InstanceCrates });
    }
    NativeModel->ClearCachedFactorGraph();
    NativeModel->OnInputOutputInterfaceChanged();

#if WITH_LIVE_CODING
    ILiveCodingModule* LiveCoding = FModuleManager::GetModulePtr<ILiveCodingModule>("LiveCoding");
    if (bLiveCoding && LiveCoding && LiveCoding->IsEnabledForSession())
    {
        LiveCoding->Compile(ELiveCodingCompileFlags::None, nullptr);
        return {};
    }
#endif
    const FString PackageName = NativeModel->GetPackage()->GetName();
    const FString Message = FString::Format(TEXT("Live Coding is not enabled. Please recompile the project from your IDE for code changes to take effect. The following files have been written:\n\n{0} (Package)\n\n{1} (Implementation)"), { PackageName, FullImplementationClassPath });
    const FText Title = INVTEXT("Nativization Files Written");
    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Message), &Title);
    return {};
}

FString FConcordModelNativization::GetImplementationCode(const FString& ClassName, const FConcordFactorGraph<float>& FactorGraph)
{
    FString ImplementationCode; ImplementationCode.Reserve(32768);
    FConcordFactorGraphTranspiler Transpiler(ImplementationCode, ClassName, FactorGraph);
    Transpiler.Transpile();
    return MoveTemp(ImplementationCode);
}

void FConcordFactorGraphTranspiler::Transpile()
{
    Out.Write(TEXT("// Auto-generated Concord model implementation\n"))
    .Write(TEXT("#pragma once\n"))
    .Write(TEXT("#include \"ConcordNativeModelImplementation.h\"\n"))
    .Write(TEXT("#include <array>\n"))
    .Write(TEXT("#include \"")).Write(ClassName).Write(TEXT(".generated.h\"\n\n"));

    Buf.Write(TEXT("UCLASS()\nclass U")).Write(ClassName).Write(TEXT(" : public UConcordNativeModelImplementation\n{\n    GENERATED_BODY()"))
    .Write(TEXT(R"handles(
    template<typename FFloatType>
    TArray<TUniquePtr<FConcordFactorHandleBase<FFloatType>>> GetHandles() const
    {
        TArray<TUniquePtr<FConcordFactorHandleBase<FFloatType>>> Handles;)handles"));
    for (const auto& Handle : FactorGraph.GetHandles())
    {
        const int32 HandleIndex = TranspileHandle(static_cast<const FHandle*>(Handle.Get()));
        Buf.Indent(2).Write(TEXT("Handles.Add(MakeUnique<")).WriteName(HandleIndex).Write(TEXT("<FFloatType>>("))
        .Indent(3).WriteStdArray(CurrentRootArgumentIndices.Num()).Write(TEXT(" ")).WriteArray(CurrentRootArgumentIndices).Write(TEXT(","))
        .Indent(3).Write(TEXT("TArray<int32> ")).WriteArray(CurrentRootNeighboringRandomVariableIndices)
        .Indent(2).Write(TEXT("));"));
    }

    bCurrentIsOutput = true;
    Buf.Write(TEXT(R"outputs(
        return MoveTemp(Handles);
    }
    template<typename FFloatType>
    typename FConcordFactorGraph<FFloatType>::FOutputs GetOutputs() const
    {
        typename FConcordFactorGraph<FFloatType>::FOutputs Outputs;)outputs"));
    for (const auto& NameOutputPair : FactorGraph.GetOutputs())
    {
        const int32 OutputIndex = TranspileOutput(NameOutputPair);
        Buf.Indent(2).Write(TEXT("Outputs.Add(\"")).Write(NameOutputPair.Key.ToString()).Write(TEXT("\", MakeUnique<")).WriteName(OutputIndex).Write(TEXT("<FFloatType>>("))
        .Indent(3).WriteStdArray(CurrentRootArgumentIndices.Num()).Write(TEXT(" ")).WriteArray(CurrentRootArgumentIndices)
        .Indent(2).Write(TEXT("));"));
    }

    Out.Write(BufStr)
    .Write(TEXT(R"overrides(
        return MoveTemp(Outputs);
    }
public:
    TArray<TUniquePtr<FConcordFactorHandleBase<float>>> GetFloatHandles() const override { return GetHandles<float>(); }
    FConcordFactorGraph<float>::FOutputs GetFloatOutputs() const override { return GetOutputs<float>(); }
};
)overrides"));
}

int32 FConcordFactorGraphTranspiler::TranspileHandle(const FHandle* Handle)
{
    CurrentRootArgumentIndices.Reset();
    CurrentRootNeighboringRandomVariableIndices.Reset();
    if (const FMergedHandle* MergedHandle = Handle->GetMergedHandle())
    {
        TArray<int32> RootIndices = TranspileComposite(MergedHandle->Children, EConcordValueType::Float);
        bool bAddedNewComposite = false;
        const FComposite& Composite = FindOrAddComposite(MergedHandles, MoveTemp(RootIndices), bAddedNewComposite);
        if (bAddedNewComposite) WriteHandle(Composite);
        return Composite.CompositeIndex;
    }
    else
    {
        const FAtomicHandle* AtomicHandle = static_cast<const FAtomicHandle*>(Handle);
        TArray<int32> RootIndices = TranspileComposite(TArrayView<const FAtomicHandle*>(&AtomicHandle, 1), EConcordValueType::Float);
        int32& HandleIndex = RootHandleMap.FindOrAdd(RootIndices[0], -1);
        if (HandleIndex != -1) return HandleIndex;
        HandleIndex = CurrentIndex++;
        WriteHandle({ HandleIndex, MoveTemp(RootIndices) });
        return HandleIndex;
    }
}

void FConcordFactorGraphTranspiler::WriteHandle(const FComposite& Composite)
{
    Out.Write(TEXT("template<typename FFloatType>"))
    .Indent(0).Write(TEXT("class ")).WriteName(Composite.CompositeIndex).Write(TEXT(" : public FConcordFactorHandle<")).WriteName(Composite.CompositeIndex).Write(TEXT("<FFloatType>, FFloatType>\n{\npublic:"))
    .Indent(1).WriteName(Composite.CompositeIndex).Write(TEXT("(")).WriteStdArray(CurrentRootArgumentIndices.Num(), true, true).Write(TEXT(" InIndices, const TArray<int32>& InNeighboringFlatRandomVariableIndices)"))
    .Indent(2).Write(TEXT(": Indices(InIndices)"))
    .Indent(1).Write(TEXT("{ NeighboringFlatRandomVariableIndices = InNeighboringFlatRandomVariableIndices; }"))
    .Indent(1).WriteStdArray(CurrentRootArgumentIndices.Num(), true).Write(TEXT(" Indices;"))
    .Indent(1).Write(TEXT("FFloatType")).WriteEvalContext().Write(TEXT(") const")).Indent(1).Write(TEXT("{"));
    if (Composite.ElementIndices.Num() == 1)
    {
        Out.Indent(2).Write(TEXT("return ")).WriteName(Composite.ElementIndices[0]).Write(TEXT("::Eval(Context, Indices.data());"));
    }
    else
    {
        Out.Indent(2).Write(TEXT("FFloatType Score = 0;"));
        if (Composite.bConstantElementIndices)
        {
            if (Composite.ElisionOffset >= 0)
            {
                Out.Indent(2).Write(TEXT("for (int32 Index = 0; Index < ")).Write(Composite.ElementIndices.Num()).Write(TEXT("; ++Index)"))
                .Indent(2).Write(TEXT("{"))
                .Indent(3).Write(TEXT("const int32 ElisionIndex = ")).Write(Composite.ElisionOffset).Write(TEXT(" + ")).Write(Composite.ElisionStride).Write(TEXT(" * Index;"))
                .Indent(3).Write(TEXT("Score += ")).WriteName(Composite.ElementIndices[0]).Write(TEXT("::Eval(Context, &ElisionIndex);"))
                .Indent(2).Write(TEXT("}"));
            }
            else
            {
                Out.Indent(2).Write(TEXT("int32 Offset = 0;"))
                .Indent(2).Write(TEXT("for (int32 Index = 0; Index < ")).Write(Composite.ElementIndices.Num()).Write(TEXT("; ++Index)"))
                .Indent(2).Write(TEXT("{"))
                .Indent(3).Write(TEXT("Score += ")).WriteName(Composite.ElementIndices[0]).Write(TEXT("::Eval(Context, Indices.data() + Offset);"))
                .Indent(3).Write(TEXT("Offset += ")).Write(ArgumentIndicesCountMap[Composite.ElementIndices[0]]).Write(TEXT(";"))
                .Indent(2).Write(TEXT("}"));
            }
        }
        else
        {
            int32 Offset = 0;
            for (int32 ExpressionIndex = 0; ExpressionIndex < Composite.ElementIndices.Num(); ++ExpressionIndex)
            {
                Out.Indent(2).Write(TEXT("Score += ")).WriteName(Composite.ElementIndices[ExpressionIndex]).Write(TEXT("::Eval(Context, Indices.data() + ")).Write(Offset).Write(TEXT(");"));
                Offset += ArgumentIndicesCountMap[Composite.ElementIndices[ExpressionIndex]];
            }
        }
        Out.Indent(2).Write(TEXT("return Score;"));
    }
    Out.Indent(1).Write(TEXT("}\n};\n"));
}

int32 FConcordFactorGraphTranspiler::TranspileOutput(const TPair<FName, TUniquePtr<FConcordFactorGraph<float>::FOutput>>& NameOutputPair)
{
    CurrentRootArgumentIndices.Reset();
    TArray<int32> RootIndices = TranspileComposite(static_cast<ConcordFactorGraphDynamic::FOutput*>(NameOutputPair.Value.Get())->Expressions, NameOutputPair.Value->GetType());
    bool bAddedNewComposite = false;
    const FComposite& Composite = FindOrAddComposite(Outputs, MoveTemp(RootIndices), bAddedNewComposite);
    if (bAddedNewComposite) WriteOutput(Composite, NameOutputPair.Value->GetType());
    return Composite.CompositeIndex;
}

void FConcordFactorGraphTranspiler::WriteOutput(const FComposite& Composite, EConcordValueType Type)
{
    Out.Write(TEXT("template<typename FFloatType>"))
    .Indent(0).Write(TEXT("class ")).WriteName(Composite.CompositeIndex).Write(TEXT(" : public FConcordFactorGraph<FFloatType>::FOutput\n{\npublic:"))
    .Indent(1).WriteName(Composite.CompositeIndex).Write(TEXT("(")).WriteStdArray(CurrentRootArgumentIndices.Num(), true, true).Write(TEXT(" InIndices)"))
    .Indent(2).Write(TEXT(": FConcordFactorGraph<FFloatType>::FOutput(")).Write(EnumString(Type)).Write(TEXT(")"))
    .Indent(2).Write(TEXT(", Indices(InIndices)"))
    .Indent(1).Write(TEXT("{}"))
    .Indent(1).WriteStdArray(CurrentRootArgumentIndices.Num(), true).Write(TEXT(" Indices;"))
    .Indent(1).Write(TEXT("void")).WriteEvalContext().Write(TEXT(", const TArrayView<")).Write(NativeTypeString(Type)).Write(TEXT(">& OutData) const override")).Indent(1).Write(TEXT("{"));
    if (Composite.bConstantElementIndices)
    {
        if (Composite.ElisionOffset >= 0)
        {
            Out.Indent(2).Write(TEXT("for (int32 Index = 0; Index < ")).Write(Composite.ElementIndices.Num()).Write(TEXT("; ++Index)"))
            .Indent(2).Write(TEXT("{"))
            .Indent(3).Write(TEXT("const int32 ElisionIndex = ")).Write(Composite.ElisionOffset).Write(TEXT(" + ")).Write(Composite.ElisionStride).Write(TEXT(" * Index;"))
            .Indent(3).Write(TEXT("OutData[Index] = ")).WriteName(Composite.ElementIndices[0]).Write(TEXT("::Eval(Context, &ElisionIndex);"))
            .Indent(2).Write(TEXT("}"));
        }
        else
        {
            Out.Indent(2).Write(TEXT("int32 Offset = 0;"))
            .Indent(2).Write(TEXT("for (int32 Index = 0; Index < ")).Write(Composite.ElementIndices.Num()).Write(TEXT("; ++Index)"))
            .Indent(2).Write(TEXT("{"))
            .Indent(3).Write(TEXT("OutData[Index] = ")).WriteName(Composite.ElementIndices[0]).Write(TEXT("::Eval(Context, Indices.data() + Offset);"))
            .Indent(3).Write(TEXT("Offset += ")).Write(ArgumentIndicesCountMap[Composite.ElementIndices[0]]).Write(TEXT(";"))
            .Indent(2).Write(TEXT("}"));
        }
    }
    else
    {
        int32 Offset = 0;
        for (int32 ExpressionIndex = 0; ExpressionIndex < Composite.ElementIndices.Num(); ++ExpressionIndex)
        {
            Out.Indent(2).Write(TEXT("OutData[")).Write(ExpressionIndex).Write(TEXT("] = "))
            .WriteName(Composite.ElementIndices[ExpressionIndex]).Write(TEXT("::Eval(Context, Indices.data() + ")).Write(Offset).Write(TEXT(");"));
            Offset += ArgumentIndicesCountMap[Composite.ElementIndices[ExpressionIndex]];
        }
    }
    Out.Indent(1).Write(TEXT("}"))
    .Indent(1).Write(TEXT("int32 Num() const override { return ")).Write(Composite.ElementIndices.Num()).Write(TEXT("; }"))
    .Indent(0).Write(TEXT("};\n"));
}

template<typename FExpression>
TArray<int32> FConcordFactorGraphTranspiler::TranspileComposite(const TArray<FExpression>& Expressions, EConcordValueType ExpectedType)
{
    return TranspileComposite(TArrayView<const FExpression>(Expressions.GetData(), Expressions.Num()), ExpectedType);
}

template<typename FExpression>
TArray<int32> FConcordFactorGraphTranspiler::TranspileComposite(const TArrayView<FExpression>& Expressions, EConcordValueType ExpectedType)
{
    TArray<int32> ElementIndices; ElementIndices.Reserve(Expressions.Num());
    for (const auto& Expression : Expressions)
    {
        const int32 PreviousIndicesCount = CurrentRootArgumentIndices.Num();
        ElementIndices.Add(TranspileExpression(Expression, ExpectedType));
        ArgumentIndicesCountMap.Add(ElementIndices.Last(), CurrentRootArgumentIndices.Num() - PreviousIndicesCount);
    }
    return MoveTemp(ElementIndices);
}

int32 FConcordFactorGraphTranspiler::TranspileExpression(const FConcordExpression* Expression, EConcordValueType ExpectedType)
{
    if (const FConcordRandomVariableExpression* RandomVariableExpression = Expression->AsRandomVariableExpression())
        return Transpile(RandomVariableExpression);
    if (const FConcordParameterExpression<int32>* IntParameterExpression = Expression->AsIntParameterExpression())
        return Transpile(IntParameterExpression);
    if (const FConcordParameterExpression<float>* FloatParameterExpression = Expression->AsFloatParameterExpression())
        return Transpile(FloatParameterExpression);
    if (const FConcordComputingExpression* ComputingExpression = Expression->AsComputingExpression())
        return Transpile(ComputingExpression, ExpectedType);
    if (const FConcordValueExpression<int32>* IntValueExpression = Expression->AsIntValueExpression())
        return Transpile(IntValueExpression);
    if (const FConcordValueExpression<float>* FloatValueExpression = Expression->AsFloatValueExpression())
        return Transpile(FloatValueExpression);
    checkNoEntry(); return {};
}

int32 FConcordFactorGraphTranspiler::TranspileExpression(const FConcordSharedExpression& Expression, EConcordValueType ExpectedType)
{
    return TranspileExpression(&Expression.Get(), ExpectedType);
}

template<typename FAtomicHandle> int32 FConcordFactorGraphTranspiler::TranspileExpression(const FAtomicHandle& AtomicHandle, EConcordValueType ExpectedType)
{
    return TranspileExpression(&AtomicHandle->Factor.Get(), ExpectedType);
}

bool FConcordFactorGraphTranspiler::GetIndex(const FString& ExpressionString, int32& IndexOut) const
{
    const int32* IndexPtr = ExpressionStringToIndex.Find(ExpressionString);
    if (IndexPtr) { IndexOut = *IndexPtr; return true; }
    return false;
}

int32 FConcordFactorGraphTranspiler::Transpile(const FConcordRandomVariableExpression* RandomVariableExpression)
{
    const FString ExpressionString = TEXT("$rand"); int32 Index;
    if (!GetIndex(ExpressionString, Index))
    {
        Index = CurrentIndex++;
        Out.WriteStructTop(Index)
        .Indent(1).WriteIndexCountEquals().Write(TEXT("1;"))
        .Indent(1).WriteTemplateEvalTop(TEXT("int32"))
        .Indent(2).Write(TEXT("return Context.Variation[Indices[0]];\n    }\n};\n"));
        ExpressionStringToIndex.Add(ExpressionString, Index);
    }
    CurrentRootArgumentIndices.Add(RandomVariableExpression->FlatIndex);
    if (!bCurrentIsOutput) CurrentRootNeighboringRandomVariableIndices.AddUnique(RandomVariableExpression->FlatIndex);
    return Index;
}

template<typename FValue>
int32 FConcordFactorGraphTranspiler::Transpile(const FConcordParameterExpression<FValue>* ParameterExpression)
{
    const FString ExpressionString = ExpressionVariableString<FValue>(); int32 Index;
    if (!GetIndex(ExpressionString, Index))
    {
        Index = CurrentIndex++;
        Out.WriteStructTop(Index)
        .Indent(1).WriteIndexCountEquals().Write(TEXT("1;"))
        .Indent(1).WriteTemplateEvalTop(NativeTypeString<FValue>());
        if constexpr (std::is_same_v<FValue, float>)
            Out.Indent(2).Write(TEXT("return ConcordFloatVar(Context.")).Write(CapitalizedTypeString<FValue>()).Write(TEXT("Parameters[Indices[0]], Indices[0]);\n    }\n};\n"));
        else
            Out.Indent(2).Write(TEXT("return Context.")).Write(CapitalizedTypeString<FValue>()).Write(TEXT("Parameters[Indices[0]];\n    }\n};\n"));
        ExpressionStringToIndex.Add(ExpressionString, Index);
    }
    CurrentRootArgumentIndices.Add(ParameterExpression->FlatIndex);
    return Index;
}

int32 FConcordFactorGraphTranspiler::Transpile(const FConcordComputingExpression* ComputingExpression, EConcordValueType ExpectedType)
{
    const FString ExpressionString = ComputingExpression->ToString();
    TSharedPtr<TArray<FExpressionVariable>> ExpressionVariables = GetComputingExpressionVariables(ExpressionString, ExpectedType);
    checkf(!ExpressionVariables->IsEmpty(), TEXT("Computing expression string must contain expression variables (e.g. $int0), use value expressions for constants."));
    const int32 MaxArgIndex = Algo::MaxElement(*ExpressionVariables, [](const auto& A, const auto& B){ return A.ArgIndex < B.ArgIndex; })->ArgIndex;
    const FString Key = ExpressionString + TEXT("$T") + NativeTypeString(ExpectedType);
    int32 TemplateIndex; if (!GetIndex(Key, TemplateIndex))
    {
        TemplateIndex = CurrentIndex++;
        Out.Write(TEXT("template")).WriteSequence(MaxArgIndex, TEXT("<"), TEXT(", "), TEXT(">\n"), [](auto& Out, int32 Index){ Out.Write(TEXT("typename FArg")).Write(Index); })
        .Write(TEXT("struct ")).WriteName(TemplateIndex).Write(TEXT(" : FConcordIndexCount"))
        .WriteSequence(MaxArgIndex, TEXT("<"), TEXT(", "), TEXT(">"), [](auto& Out, int32 Index){ Out.Write(TEXT("FArg")).Write(Index); })
        .Write(TEXT("\n{"))
        .Indent(1).WriteTemplateEvalTop(NativeTypeString(ExpectedType))
        .Write(*ExpressionString, (*ExpressionVariables)[0].Begin);
        for (int32 ExpressionVariableIndex = 0; ExpressionVariableIndex < ExpressionVariables->Num(); ++ExpressionVariableIndex)
        {
            const FExpressionVariable& ExpressionVariable = (*ExpressionVariables)[ExpressionVariableIndex];
            Out.Write(TEXT("FArg")).Write(ExpressionVariable.ArgIndex).Write(TEXT("::Eval(Context, Indices"))
            .WriteSequence(ExpressionVariable.ArgIndex - 1, TEXT(" + "), TEXT(" + "), TEXT(""), [](auto& Out, int32 Index){ Out.Write(TEXT("FArg")).Write(Index).Write(TEXT("::IndexCount")); })
            .Write(ExpressionVariable.SourceExpressionArrayNum != 0 ? TEXT(", ") : TEXT(")"));
            const int32 StretchEnd = (ExpressionVariableIndex < ExpressionVariables->Num() - 1) ? (*ExpressionVariables)[ExpressionVariableIndex + 1].Begin : ExpressionString.Len();
            Out.Write(*ExpressionString + ExpressionVariable.End, StretchEnd - ExpressionVariable.End);
        }
        Out.Write(TEXT("    }\n};\n"));
        ExpressionStringToIndex.Add(Key, TemplateIndex);
    }

    TArray<int32> ArgumentInstanceIndices;
    for (int32 ArgIndex = 0; ArgIndex <= MaxArgIndex; ++ArgIndex)
    {
        const FExpressionVariable* ExpressionVariable = ExpressionVariables->FindByPredicate([=](const auto& X){ return X.ArgIndex == ArgIndex; });
        if (ExpressionVariable->SourceExpressionArrayNum != 0) ArgumentInstanceIndices.Add(TranspileArray(ComputingExpression, ExpressionVariable));
        else ArgumentInstanceIndices.Add(TranspileExpression(ComputingExpression->SourceExpressions[ExpressionVariable->SourceExpressionIndex], ExpressionVariable->ExpectedType));
    }
    TArray<FInstantiation>& Instantiations = TemplateInstantiations.FindOrAdd(TemplateIndex);
    for (const FInstantiation& Instantiation : Instantiations)
        if (Instantiation.ArgumentIndices == ArgumentInstanceIndices)
            return Instantiation.InstanceIndex;
    FInstantiation& Instantiation = Instantiations.AddDefaulted_GetRef();
    Instantiation.InstanceIndex = CurrentIndex++;
    Instantiation.ArgumentIndices = MoveTemp(ArgumentInstanceIndices);
    Out.Write(TEXT("using ")).WriteName(Instantiation.InstanceIndex).Write(TEXT(" = ")).WriteName(TemplateIndex);
    if (!Instantiation.ArgumentIndices.IsEmpty())
        Out.WriteArray(Instantiation.ArgumentIndices, TEXT("<"), TEXT(", "), TEXT(">"), [](auto& Out, int32 Index){ Out.WriteName(Index); });
    Out.Write(TEXT(";\n"));
    return Instantiation.InstanceIndex;
}

TSharedPtr<TArray<FConcordFactorGraphTranspiler::FExpressionVariable>> FConcordFactorGraphTranspiler::GetComputingExpressionVariables(const FString& ExpressionString, EConcordValueType ExpectedType)
{
    TSharedPtr<TArray<FExpressionVariable>>& ExpressionVariables = ParsedComputingExpressionStrings.FindOrAdd(ExpressionString);
    if (ExpressionVariables) return ExpressionVariables;
    ExpressionVariables = MakeShared<TArray<FExpressionVariable>>();
    FRegexMatcher RegexMatcher(FRegexPattern(TEXT("\\$(int|float|any)(\\d+)(array(\\d*)\\()?")), ExpressionString);
    while (RegexMatcher.FindNext())
    {
        FExpressionVariable& ExpressionVariable = ExpressionVariables->AddDefaulted_GetRef();
        ExpressionVariable.Begin = RegexMatcher.GetMatchBeginning();
        ExpressionVariable.End = RegexMatcher.GetMatchEnding();
        ExpressionVariable.ExpectedType = ExpectedType;
        if (ExpressionString[RegexMatcher.GetCaptureGroupBeginning(1)] == 'i') ExpressionVariable.ExpectedType = EConcordValueType::Int;
        else if (ExpressionString[RegexMatcher.GetCaptureGroupBeginning(1)] == 'f') ExpressionVariable.ExpectedType = EConcordValueType::Float;
        LexFromString(ExpressionVariable.SourceExpressionIndex, *RegexMatcher.GetCaptureGroup(2));
        if (RegexMatcher.GetCaptureGroupBeginning(3) != RegexMatcher.GetCaptureGroupEnding(3)) // array
        {
            if (RegexMatcher.GetCaptureGroupBeginning(4) != RegexMatcher.GetCaptureGroupEnding(4))
                LexFromString(ExpressionVariable.SourceExpressionArrayNum, *RegexMatcher.GetCaptureGroup(4));
            else
                ExpressionVariable.SourceExpressionArrayNum = -1;
        }
        else ExpressionVariable.SourceExpressionArrayNum = 0;

        ExpressionVariable.ArgIndex = 0;
        for (int32 Index = 0; Index < ExpressionVariables->Num() - 1; ++Index)
        {
            const FExpressionVariable& OtherExpressionVariable = (*ExpressionVariables)[Index];
            if (ExpressionVariable.SourceExpressionIndex == OtherExpressionVariable.SourceExpressionIndex &&
                ExpressionVariable.SourceExpressionArrayNum == OtherExpressionVariable.SourceExpressionArrayNum)
            {
                ExpressionVariable.ArgIndex = OtherExpressionVariable.ArgIndex;
                break;
            }
            if (OtherExpressionVariable.ArgIndex >= ExpressionVariable.ArgIndex)
                ExpressionVariable.ArgIndex = OtherExpressionVariable.ArgIndex + 1;
        }
    }
    return ExpressionVariables;
}

int32 FConcordFactorGraphTranspiler::TranspileArray(const FConcordComputingExpression* ComputingExpression, const FExpressionVariable* ExpressionVariable)
{
    const int32 CurrentRootArgumentIndicesOffset = CurrentRootArgumentIndices.Num();
    const FConcordSharedExpression* StartExpression = ComputingExpression->SourceExpressions.GetData() + ExpressionVariable->SourceExpressionIndex;
    int32 ArrayNum = ExpressionVariable->SourceExpressionArrayNum;
    if (ArrayNum == -1) ArrayNum = ComputingExpression->SourceExpressions.Num() - ExpressionVariable->SourceExpressionIndex;
    TArray<int32> ElementIndices = TranspileComposite(TArrayView<const FConcordSharedExpression>(StartExpression, ArrayNum), ExpressionVariable->ExpectedType);
    if (ElementIndices.IsEmpty()) { Out.Write(TEXT("$ERROR: Empty Array in Expression String: ### ")).Write(ComputingExpression->ToString()).Write("###"); return CurrentIndex++; }
    bool bAddedNewComposite = false;
    const FComposite& Composite = FindOrAddComposite(Arrays, MoveTemp(ElementIndices), bAddedNewComposite, CurrentRootArgumentIndicesOffset);
    if (bAddedNewComposite) WriteArray(Composite, ExpressionVariable->ExpectedType);
    return Composite.CompositeIndex;
}

void FConcordFactorGraphTranspiler::WriteArray(const FComposite& Composite, EConcordValueType Type)
{
    int32 TotalIndicesCount = 0;
    if (Composite.ElisionOffset < 0) for (int32 ElementIndex : Composite.ElementIndices) TotalIndicesCount += ArgumentIndicesCountMap[ElementIndex];
    Out.WriteStructTop(Composite.CompositeIndex)
    .Indent(1).WriteIndexCountEquals().Write(TotalIndicesCount).Write(TEXT(";"))
    .Indent(1).WriteTemplateEvalTop(NativeTypeString(Type), true);
    if (Composite.bConstantElementIndices)
    {
        if (Composite.ElisionOffset >= 0)
        {
            Out.Indent(2).Write(TEXT("const int32 ElisionIndex = ")).Write(Composite.ElisionOffset).Write(TEXT(" + ")).Write(Composite.ElisionStride).Write(TEXT(" * Index;"))
            .Indent(2).Write(TEXT("return ")).WriteName(Composite.ElementIndices[0]).Write(TEXT("::Eval(Context, &ElisionIndex);"));
        }
        else
        {
            Out.Indent(2).Write(TEXT("return ")).WriteName(Composite.ElementIndices[0]).Write(TEXT("::Eval(Context, Indices + Index * "))
            .Write(ArgumentIndicesCountMap[Composite.ElementIndices[0]]).Write(TEXT(");"));
        }
    }
    else
    {
        Out.Indent(2).Write(TEXT("switch (Index)")).Indent(2).Write(TEXT("{"));
        int32 Offset = 0;
        for (int32 ExpressionIndex = 0; ExpressionIndex < Composite.ElementIndices.Num(); ++ExpressionIndex)
        {
            Out.Indent(2).Write(TEXT("case ")).Write(ExpressionIndex).Write(TEXT(": return "))
            .WriteName(Composite.ElementIndices[ExpressionIndex]).Write(TEXT("::Eval(Context, Indices + ")).Write(Offset).Write(TEXT(");"));
            Offset += ArgumentIndicesCountMap[Composite.ElementIndices[ExpressionIndex]];
        }
        Out.Indent(2).Write(TEXT("default: checkNoEntry(); return 0;")).Indent(2).Write(TEXT("}"));
    }
    Out.Indent(1).Write(TEXT("}\n};\n"));
}

template<typename FValue>
int32 FConcordFactorGraphTranspiler::Transpile(const FConcordValueExpression<FValue>* ValueExpression)
{
    const FString ValueString = LexToString(ValueExpression->Value);
    const FString ExpressionString = TEXT("$v") + ValueString; int32 Index;
    if (!GetIndex(ExpressionString, Index))
    {
        Index = CurrentIndex++;
        Out.WriteStructTop(Index)
        .Indent(1).WriteIndexCountEquals().Write(TEXT("0;"))
        .Indent(1).WriteTemplateEvalTop(NativeTypeString<FValue>())
        .Indent(2).Write(TEXT("return ")).Write(*ValueString).Write(TEXT(";\n    }\n};\n"));
        ExpressionStringToIndex.Add(ExpressionString, Index);
    }
    return Index;
}

bool FConcordFactorGraphTranspiler::IsConstant(const TArray<int32>& ElementIndices)
{
    check(!ElementIndices.IsEmpty());
    const int32 RootIndex = ElementIndices[0];
    for (int32 Index = 1; Index < ElementIndices.Num(); ++Index)
        if (ElementIndices[Index] != RootIndex)
            return false;
    return true;
}

void FConcordFactorGraphTranspiler::GetElisionInfoForConstantElementIndex(int32 ElementIndex, int32 CurrentRootArgumentIndicesOffset, int32& OutElisionOffset, int32& OutElisionStride)
{
    if (ArgumentIndicesCountMap[ElementIndex] > 1) { OutElisionOffset = -1; return; } // do not elide when the element needs more than 1 index
    if (CurrentRootArgumentIndices.Num() - CurrentRootArgumentIndicesOffset < 5) { OutElisionOffset = -1; return; } // do not elide when there are only 4 indices or less in total

    OutElisionOffset = CurrentRootArgumentIndices[CurrentRootArgumentIndicesOffset];
    OutElisionStride = CurrentRootArgumentIndices[CurrentRootArgumentIndicesOffset + 1] - OutElisionOffset;
    for (int32 CurrentRootArgumentIndicesIndex = CurrentRootArgumentIndicesOffset + 2; CurrentRootArgumentIndicesIndex < CurrentRootArgumentIndices.Num(); ++CurrentRootArgumentIndicesIndex)
        if (CurrentRootArgumentIndices[CurrentRootArgumentIndicesIndex] - CurrentRootArgumentIndices[CurrentRootArgumentIndicesIndex - 1] != OutElisionStride)
        {
            OutElisionOffset = -1;
            return;
        }
    CurrentRootArgumentIndices.SetNum(CurrentRootArgumentIndicesOffset);
    return;
}

FConcordFactorGraphTranspiler::FComposite& FConcordFactorGraphTranspiler::FindOrAddComposite(TArray<FComposite>& Composites, TArray<int32>&& ElementIndices, bool& bOutAddedNewComposite, int32 CurrentRootArgumentIndicesOffset)
{
    const bool bConstantElementIndices = IsConstant(ElementIndices);
    int32 ElisionOffset = -1, ElisionStride;
    if (bConstantElementIndices) GetElisionInfoForConstantElementIndex(ElementIndices[0], CurrentRootArgumentIndicesOffset, ElisionOffset, ElisionStride);

    bOutAddedNewComposite = false;
    FComposite* ExisitingComposite = Composites.FindByPredicate([&](const auto& OtherComposite)
    {
        if (bConstantElementIndices != OtherComposite.bConstantElementIndices) return false;
        if (ElisionOffset != OtherComposite.ElisionOffset) return false;
        if (ElisionOffset >= 0 && ElisionStride != OtherComposite.ElisionStride) return false;
        if (bConstantElementIndices) return ElementIndices.Num() == OtherComposite.ElementIndices.Num() && ElementIndices[0] == OtherComposite.ElementIndices[0];
        return ElementIndices == OtherComposite.ElementIndices;
    });
    if (ExisitingComposite) return *ExisitingComposite;

    bOutAddedNewComposite = true;
    FComposite& Composite = Composites.AddDefaulted_GetRef();
    Composite.CompositeIndex = CurrentIndex++;
    Composite.ElementIndices = MoveTemp(ElementIndices);
    Composite.bConstantElementIndices = bConstantElementIndices;
    Composite.ElisionOffset = ElisionOffset;
    Composite.ElisionStride = ElisionStride;
    return Composite;
}

template<typename FStringType> FConcordFactorGraphTranspiler::FOutput& FConcordFactorGraphTranspiler::FOutput::Write(const FStringType& String)
{
    Out.Append(String);
    return *this;
}

FConcordFactorGraphTranspiler::FOutput& FConcordFactorGraphTranspiler::FOutput::Write(const TCHAR* String, int32 Count)
{
    Out.Append(String, Count);
    return *this;
}

FConcordFactorGraphTranspiler::FOutput& FConcordFactorGraphTranspiler::FOutput::Write(bool bCondition, const TCHAR* String)
{
    if (bCondition) Out.Append(String);
    return *this;
}

FConcordFactorGraphTranspiler::FOutput& FConcordFactorGraphTranspiler::FOutput::Write(int32 Index)
{
    Out.AppendInt(Index);
    return *this;
}

FConcordFactorGraphTranspiler::FOutput& FConcordFactorGraphTranspiler::FOutput::WriteName(int32 Index)
{
    return Write(TEXT("F")).Write(*ClassName).Write(TEXT("__")).Write(Index);
}

FConcordFactorGraphTranspiler::FOutput& FConcordFactorGraphTranspiler::FOutput::WriteStructTop(int32 Index)
{
    return Write(TEXT("struct ")).WriteName(Index).Write(TEXT("\n{"));
}

FConcordFactorGraphTranspiler::FOutput& FConcordFactorGraphTranspiler::FOutput::WriteEvalContext()
{
    return Write(TEXT(" Eval(const FConcordExpressionContext<FFloatType>& Context"));
}

FConcordFactorGraphTranspiler::FOutput& FConcordFactorGraphTranspiler::FOutput::WriteTemplateEvalTop(const TCHAR* ReturnType, bool bIsArray)
{
    return Write(TEXT("template<typename FFloatType>"))
    .Indent(1).Write(TEXT("static ")).Write(ReturnType).WriteEvalContext().Write(TEXT(", const int32 Indices[]"))
    .Write(bIsArray, TEXT(", int32 Index")).Write(TEXT(")")).Indent(1).Write(TEXT("{"));
}

FConcordFactorGraphTranspiler::FOutput& FConcordFactorGraphTranspiler::FOutput::WriteIndexCountEquals()
{
    return Write(TEXT("static constexpr int32 IndexCount = "));
}

template<typename FElem, typename FElemToString>
FConcordFactorGraphTranspiler::FOutput& FConcordFactorGraphTranspiler::FOutput::WriteArray(const TArray<FElem>& Array, const TCHAR* Open, const TCHAR* Combine, const TCHAR* Close, const FElemToString& ElemToStringFunc)
{
    Out.Append(Open);
    for (int32 Index = 0; Index < Array.Num() - 1; ++Index)
    {
        ElemToStringFunc(*this, Array[Index]);
        Out.Append(Combine);
    }
    if (!Array.IsEmpty()) ElemToStringFunc(*this, Array.Last());
    Out.Append(Close);
    return *this;
}

FConcordFactorGraphTranspiler::FOutput& FConcordFactorGraphTranspiler::FOutput::WriteArray(const TArray<int32>& Array)
{
    return WriteArray(Array, TEXT("{ "), TEXT(", "), TEXT(" }"), [](auto& Out, int32 Index){ Out.Write(Index); });
}

template<typename FIntToString>
FConcordFactorGraphTranspiler::FOutput& FConcordFactorGraphTranspiler::FOutput::WriteSequence(int32 Max, const TCHAR* Open, const TCHAR* Combine, const TCHAR* Close, const FIntToString& IntToStringFunc)
{
    if (Max < 0) return *this;
    Out.Append(Open);
    for (int32 Index = 0; Index < Max; ++Index)
    {
        IntToStringFunc(*this, Index);
        Out.Append(Combine);
    }
    IntToStringFunc(*this, Max);
    Out.Append(Close);
    return *this;
}

FConcordFactorGraphTranspiler::FOutput& FConcordFactorGraphTranspiler::FOutput::WriteStdArray(int32 Size, bool bConst, bool bReference)
{
    return Write(bConst, TEXT("const ")).Write(TEXT("std::array<int32, ")).Write(Size).Write(TEXT(">")).Write(bReference, TEXT("&"));
}

FConcordFactorGraphTranspiler::FOutput& FConcordFactorGraphTranspiler::FOutput::Indent(int32 Level)
{
    Write(TEXT("\n"));
    for (int32 Index = 0; Index < Level * 4; ++Index) Write(TEXT(" "));
    return *this;
}
