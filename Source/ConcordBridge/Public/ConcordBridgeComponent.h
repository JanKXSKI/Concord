// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Common/UdpSocketReceiver.h"
#include "Containers/Queue.h"
#include "ConcordCrate.h"
#include "ConcordPattern.h"
#include "ConcordBridgeComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogConcordBridgeComponent, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConcordCrateReceived, const FConcordCrateData&, CrateData);

UCLASS(meta=(BlueprintSpawnableComponent))
class CONCORDBRIDGE_API UConcordBridgeComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UConcordBridgeComponent();

    UPROPERTY(EditAnywhere, Category = "Concord Bridge")
    FString SendIPAddress;

    UPROPERTY(EditAnywhere, Category = "Concord Bridge")
    int32 SendPort;

    UPROPERTY(EditAnywhere, Category = "Concord Bridge")
    FString ReceiveIPAddress;

    UPROPERTY(EditAnywhere, Category = "Concord Bridge")
    int32 ReceivePort;

    UPROPERTY(BlueprintAssignable, Category = "Concord Bridge")
    FOnConcordCrateReceived OnCrateReceived;

    UFUNCTION(BlueprintCallable, Category = "Concord Bridge")
    void SendPattern(const FConcordPatternData& PatternData);

    void BeginPlay() override;
    void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
    void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    FSocket* ClientSocket;
    TSharedPtr<FInternetAddr> SendInternetAddr;
    FSocket* ServerSocket;
    TUniquePtr<FUdpSocketReceiver> SocketReceiver;
    bool bSetupSuccessful;

    void OnDataReceived(const FArrayReaderPtr& InData, const FIPv4Endpoint& InEndpoint);
    TQueue<FConcordCrateData> ReceivedCrates;
};
