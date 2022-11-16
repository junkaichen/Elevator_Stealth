// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "GameFramework/GameModeBase.h"
#include "Networking.h"
#include "Runtime/Core/Public/Containers/UnrealString.h"
#include "YuriTest.generated.h"

/**
 * 
 */
UCLASS()
class ELEVATOR_STEALTH_API AYuriTest : public AGameModeBase
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
private:
	FSocket* ListenerSocket;
	FSocket* ConnectionSocket;
	FTimerHandle timerHandle;
	void SocketListener();
	void ParseMessage(FString msg);
	void SendLogin();
	bool SendString(FString msg);
	void ReceivedChatMessage(FString UserName, FString message);
	
};
