// Copyright Jan Klimaschewski. All Rights Reserved.

#include "ConcordModelFactory.h"
#include "ConcordModel.h"
#include "ConcordModelGraph.h"

UConcordModelFactory::UConcordModelFactory()
{
    SupportedClass = UConcordModel::StaticClass();
    bCreateNew = true;
}

UObject* UConcordModelFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
    UConcordModel* ConcordModel = NewObject<UConcordModel>(InParent, Class, Name, Flags, Context);
    ConcordModel->Graph = NewObject<UConcordModelGraph>(ConcordModel, FName(), RF_Transactional);
    ConcordModel->Graph->Schema = UConcordModelGraphSchema::StaticClass();
    return ConcordModel;
}
