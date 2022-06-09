// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "ConcordModelFactory.generated.h"

UCLASS()
class UConcordModelFactory : public UFactory
{
    GENERATED_BODY()
public:
    UConcordModelFactory();
    UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
