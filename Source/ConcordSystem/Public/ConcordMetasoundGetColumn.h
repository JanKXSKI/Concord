// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "ConcordMetasoundPatternAsset.h"
#include "Internationalization/Text.h"
#include "MetasoundExecutableOperator.h"
#include "MetasoundNodeRegistrationMacro.h"
#include "MetasoundParamHelper.h"
#include "MetasoundPrimitives.h"
#include "MetasoundStandardNodesNames.h"
#include "MetasoundStandardNodesCategories.h"
#include "MetasoundFacade.h"
#include "CoreMinimal.h"
#include "MetasoundNode.h"
#include "MetasoundBuilderInterface.h"
#include "MetasoundTrigger.h"
#include "MetasoundNodeInterface.h"
#include "MetasoundVertex.h"
#include <type_traits>

namespace Metasound
{
    class CONCORDSYSTEM_API FConcordGetColumnNode : public FNodeFacade
    {
	public:
		FConcordGetColumnNode(const FNodeInitData& InitData);
		virtual ~FConcordGetColumnNode() = default;
	};
   

    class CONCORDSYSTEM_API FConcordGetColumnOperator : public TExecutableOperator<FConcordGetColumnOperator>
    {	
    public:

        static const FNodeClassMetadata& GetNodeInfo();
        static const FVertexInterface& GetVertexInterface();
        static TUniquePtr<IOperator> CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors);
        
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
