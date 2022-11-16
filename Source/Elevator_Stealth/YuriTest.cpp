// Fill out your copyright notice in the Description page of Project Settings.


#include "YuriTest.h"
#include <string>

void AYuriTest::BeginPlay()
{
	Super::BeginPlay();

	FIPv4Endpoint Endpoint(FIPv4Address(127, 0, 0, 1), 6667);
	ListenerSocket = FTcpSocketBuilder(TEXT("TwitchListener"))
		.AsReusable()
		.BoundToEndpoint(Endpoint)
		.Listening(8);

	//Set Buffer Size
	int32 NewSize = 0;
	ListenerSocket->SetReceiveBufferSize(2 * 1024 * 1024, NewSize);

	GetWorldTimerManager().SetTimer(timerHandle, this, &AYuriTest::SocketListener, 0.01, true);


	SendLogin();
}

void AYuriTest::SocketListener()
{
	TArray<uint8> ReceivedData;
	uint32 Size;
	bool Received = false;
	while (ListenerSocket->HasPendingData(Size))
	{
		Received = true;
		ReceivedData.SetNumUninitialized(FMath::Min(Size, 65507u));

		int32 Read = 0;
		ListenerSocket->Recv(ReceivedData.GetData(), ReceivedData.Num(), Read);
	}
	if (Received)
	{
		/*const std::string cstr(reinterpret_cast<const char*>(ReceivedData.GetData()), ReceivedData.Num());
		FString fs(cstr.c_str());*/
		ReceivedData.Add(0);
		FString fs = FString(UTF8_TO_TCHAR(ReceivedData.GetData()));
		ParseMessage(fs);
	}
}

void AYuriTest::ParseMessage(FString msg)
{
	//UE_LOG(LogTemp, Warning, TEXT("%s"), *msg);
	TArray<FString> lines;
	msg.ParseIntoArrayLines(lines);
	for (FString fs : lines)
	{
		TArray<FString> parts;
		fs.ParseIntoArray(parts, TEXT(":"));
		TArray<FString> meta;
		parts[0].ParseIntoArrayWS(meta);
		if (parts.Num() >= 2)
		{
			if (meta[0] == TEXT("PING"))
			{

			}
			else if (meta.Num() == 3 && meta[1] == TEXT("PRIVMSG"))
			{
				FString message = parts[1];
				if (parts.Num() > 2)
				{
					for (int i = 2; i < parts.Num(); i++)
					{
						message += TEXT(":") + parts[i];
					}
				}
				FString username;
				FString tmp;
				meta[0].Split(TEXT("!"), &username, &tmp);
				ReceivedChatMessage(username, message);
				continue;
			}
		}
	}
}

void AYuriTest::ReceivedChatMessage(FString UserName, FString message)
{
	UE_LOG(LogTemp, Warning, TEXT("%s: %s"), *UserName, *message);
}

void AYuriTest::SendLogin()
{
	UE_LOG(LogTemp, Warning, TEXT("StartConnect"));
	FResolveInfo* ResolveInfo = ISocketSubsystem::Get()->GetHostByName("irc.chat.twitch.tv");
	while (!ResolveInfo->IsComplete())
	{

		UE_LOG(LogTemp, Warning, TEXT("connecting..."));
	}
	if (ResolveInfo->GetErrorCode() != 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Unable to connect"));
		return;
	}

	const FInternetAddr* Addr = &ResolveInfo->GetResolvedAddress();
	uint32 OutIP = 0;
	Addr->GetIp(OutIP);
	int32 port = 6667;

	TSharedRef<FInternetAddr> addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	addr->SetIp(OutIP);
	addr->SetPort(port);

	ListenerSocket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("default"), false);

	bool connected = ListenerSocket->Connect(*addr);
	if (!connected)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to connect."));
		if (ListenerSocket)
			ListenerSocket->Close();
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("Connected"));

	SendString(TEXT("PASS oauth:3w7hfsrxaif38j1hop2klbriukpq9r\r\nNICK neji_yuri\r\nUSER neji_yuri\r\nJOIN #neji_yuri\r\n"));
	/*SendString(TEXT("NICK neji_yuri"));
	SendString(TEXT("USER neji_yuri 8 * :neji_yuri"));*/
	//SendString(TEXT("JOIN #neji_yuri"));
}

bool AYuriTest::SendString(FString msg)
{
	FString serialized = msg + TEXT("");
	TCHAR* serializedChar = serialized.GetCharArray().GetData();
	int32 size = FCString::Strlen(serializedChar);
	int32 sent = 0;

	return ListenerSocket->Send((uint8*)TCHAR_TO_UTF8(serializedChar), size, sent);

	return true;
}

