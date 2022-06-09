// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#include "ConcordBridgeComponent.h"
#include "Common/UdpSocketBuilder.h"
#include "JsonObjectConverter.h"
#include "GenericPlatform/GenericPlatformString.h"

DEFINE_LOG_CATEGORY(LogConcordBridgeComponent);

UConcordBridgeComponent::UConcordBridgeComponent()
    : SendIPAddress("127.0.0.1")
    , SendPort(7001)
    , ReceiveIPAddress("127.0.0.1")
    , ReceivePort(7000)
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UConcordBridgeComponent::SendPattern(const FConcordPatternData& PatternData)
{
    if (!bSetupSuccessful) { UE_LOG(LogConcordBridgeComponent, Error, TEXT("Tried to send pattern but component was not successfully setup.")); return; }
    FString PatternString;
    if (!FJsonObjectConverter::UStructToJsonObjectString(PatternData, PatternString, 0, 0, 0, nullptr, false)) { checkNoEntry(); return; }
    const int32 UTF8Len = FGenericPlatformString::ConvertedLength<UTF8CHAR>(*PatternString, PatternString.Len());
    TArray<uint8> Data; Data.SetNumUninitialized(UTF8Len);
    FGenericPlatformString::Convert((UTF8CHAR*)Data.GetData(), Data.Num(), *PatternString, PatternString.Len());
    TArrayView<uint8> DataView = MakeArrayView(Data);
    int32 BytesSent = 0;
    while (DataView.Num() > 0)
    {
        const bool bSuccess = ClientSocket->SendTo(DataView.GetData(), DataView.Num(), BytesSent, *SendInternetAddr);
        if (!bSuccess || BytesSent <= 0) { UE_LOG(LogConcordBridgeComponent, Error, TEXT("Failed to send pattern.")); return; }
        DataView.RightChopInline(BytesSent);
    }
}

void UConcordBridgeComponent::BeginPlay()
{
	bSetupSuccessful = false;

    ClientSocket = FUdpSocketBuilder(TEXT("ConcordBridgeClient")).Build();
    SendInternetAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
    bool bIsValidAddress = true;
    SendInternetAddr->SetIp(*SendIPAddress, bIsValidAddress);
    SendInternetAddr->SetPort(SendPort);
    if (bIsValidAddress) { UE_LOG(LogConcordBridgeComponent, Log, TEXT("Concord Bridge Client IP Address: %s:%d."), *SendIPAddress, SendPort); }
    else { UE_LOG(LogConcordBridgeComponent, Error, TEXT("Concord Bridge Client IP Address: %s:%d failed to set."), *SendIPAddress, SendPort); return; }

	FUdpSocketBuilder ServerSocketBuilder(TEXT("ConcordBridgeServer"));
	FIPv4Address ReceiveIPv4Address;
	if (!FIPv4Address::Parse(ReceiveIPAddress, ReceiveIPv4Address)) { UE_LOG(LogConcordBridgeComponent, Error, TEXT("Invalid ReceiveIPAddress: %s."), *ReceiveIPAddress); return; }
	else { UE_LOG(LogConcordBridgeComponent, Log, TEXT("Concord Bridge Server IP Address: %s:%d."), *ReceiveIPAddress, ReceivePort); }
	ServerSocketBuilder.BoundToAddress(ReceiveIPv4Address);
	ServerSocketBuilder.BoundToPort(ReceivePort);
	ServerSocket = ServerSocketBuilder.Build();
	if (!ServerSocket) { UE_LOG(LogConcordBridgeComponent, Error, TEXT("Failed to build receiving socket listening on %s:%d."), *ReceiveIPAddress, ReceivePort); return; }
	SocketReceiver = MakeUnique<FUdpSocketReceiver>(ServerSocket, FTimespan::FromMilliseconds(50), TEXT("ConcordBridgeServer_ListenerThread"));
	SocketReceiver->OnDataReceived().BindUObject(this, &UConcordBridgeComponent::OnDataReceived);
	SocketReceiver->Start();

    bSetupSuccessful = true;
    Super::BeginPlay();
}

void UConcordBridgeComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    FConcordCrateData CrateData;
    while (ReceivedCrates.Dequeue(CrateData)) OnCrateReceived.Broadcast(CrateData);
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UConcordBridgeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (ClientSocket) { ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ClientSocket); ClientSocket = nullptr; }
	if (SocketReceiver) SocketReceiver.Reset();
    if (ServerSocket) { ServerSocket->Close(); ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ServerSocket); ServerSocket = nullptr; }
    bSetupSuccessful = false;
    Super::EndPlay(EndPlayReason);
}

void UConcordBridgeComponent::OnDataReceived(const FArrayReaderPtr& InData, const FIPv4Endpoint& InEndpoint)
{
    FString CrateString(InData->Num(), (UTF8CHAR*)InData->GetData());
    FConcordCrateData CrateData;
    if (FJsonObjectConverter::JsonObjectStringToUStruct(CrateString, &CrateData)) ReceivedCrates.Enqueue(MoveTemp(CrateData));
}
