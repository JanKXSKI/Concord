// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MetasoundNode.h"
#include "MetasoundBuilderInterface.h"
#include "MetasoundExecutableOperator.h"
#include "MetasoundPrimitives.h"
#include "MetasoundTrigger.h"
#include "ConcordMetasoundPatternAsset.h"

namespace Metasound
{
    class CONCORDSYSTEM_API FConcordGetColumnNode : public FNode
    {
        class FOperatorFactory : public IOperatorFactory
        {
            virtual TUniquePtr<IOperator> CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors) override;
        };
    public:
        static const FVertexInterface& DeclareVertexInterface();
        static const FNodeClassMetadata& GetNodeInfo();

        FConcordGetColumnNode(const FVertexName& InName, const FGuid& InInstanceID);
        FConcordGetColumnNode(const FNodeInitData& InInitData);
        virtual ~FConcordGetColumnNode() = default;

        virtual FOperatorFactorySharedRef GetDefaultOperatorFactory() const override { return Factory; }
        virtual const FVertexInterface& GetVertexInterface() const override { return Interface; }
        virtual bool SetVertexInterface(const FVertexInterface& InInterface) override { return InInterface == Interface; }
        virtual bool IsVertexInterfaceSupported(const FVertexInterface& InInterface) const override { return InInterface == Interface; }
    private:
        FOperatorFactorySharedRef Factory;
        FVertexInterface Interface;
    };

    class CONCORDSYSTEM_API FConcordGetColumnOperator : public TExecutableOperator<FConcordGetColumnOperator>
    {	
    public:
        FConcordGetColumnOperator(const FTriggerReadRef& InTrigger,
                                  const FConcordMetasoundPatternAssetReadRef& InPatternAsset,
                                  const TDataReadReference<FString>& InColumnPath,
                                  const FInt32ReadRef& InColumnIndexOverride)
            : Trigger(InTrigger)
            , PatternAsset(InPatternAsset)
            , ColumnPath(InColumnPath)
            , ColumnIndexOverride(InColumnIndexOverride)
            , Column(TDataWriteReference<TArray<int32>>::CreateNew())
        {}

        virtual FDataReferenceCollection GetInputs() const override;
        virtual FDataReferenceCollection GetOutputs() const override;

        void Execute();
        void SetColumn(const TArray<int32>& Values);
    private:
        FTriggerReadRef Trigger;
        FConcordMetasoundPatternAssetReadRef PatternAsset;
        TDataReadReference<FString> ColumnPath;
        FInt32ReadRef ColumnIndexOverride;

        TDataWriteReference<TArray<int32>> Column;
    };
}
