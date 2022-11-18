#pragma once
#include "CoreMinimal.h"
#include "Sockets.h"
#include "Networking.h"
#include <iostream>
#include <string>

class SocketAdapter
{
public:
	SocketAdapter();
	~SocketAdapter();





	void sendServerMessageWithNoResponse(std::string &message_val);

	std::string sendServerMessageWithResponse(std::string &aouth);

	void closeSocket();

	int receiveMessage(std::string & str);

	void cleanupSocket();

	bool checkInternetConnectionHealth();

	bool checkTwitchConnectionHealth();

	std::string establishServerConnection(const std::string serverAdress,const std::string serverPort);

private:
	/*Shortand to avoit having to call ISocketSubsystem::Get() every time*/
	ISocketSubsystem* _socketSubSystem;
	/*Socket used to talk with the twich chat*/
	FSocket* _unrealSocket;

};

