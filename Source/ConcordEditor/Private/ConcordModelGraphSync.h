// Copyright Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UConcordModelGraphConsumer;
class UEdGraphPin;
class UConcordModel;
class UConcordVertex;
class UConcordTransformer;

class FConcordModelGraphSync
{
public:
    static void SyncModelConnections(UConcordModelGraphConsumer* VertexNode);
    static void SyncModelConnections(UConcordModelGraphConsumer* VertexNode, int32 PinIndex);
private:
    static UConcordTransformer* MakeDefaultValue(UConcordVertex* Vertex, int32 PinIndex, UEdGraphPin* Pin);
    static UConcordTransformer* GetOwningTransformer(const UEdGraphPin* Pin);
};
