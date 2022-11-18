#include "SocketAdapter.h"
/*The size of the buffer used during the connection to chat it's ok to always use the bigger one because you can receive a message and it not take the whole buffer while still being able to keep going*/
const int32 CONNECTION_BUFFER_SIZE = 1024;

/*
The size of the buffer to receive chat messages.
*/
const int SERVER_ADAPTER_BUFFER_SIZE = 1024;

SocketAdapter::SocketAdapter()
{
	_socketSubSystem = ISocketSubsystem::Get("WINDOWS");
	UE_LOG(LogTemp, Warning, TEXT("Created new twitch adapter to channel %s"), _socketSubSystem->GetSocketAPIName());
	TSharedPtr<FInternetAddr> remoteAdress = _socketSubSystem->CreateInternetAddr();
	_unrealSocket = _socketSubSystem->CreateSocket(NAME_Stream, "twitchIntegratorSocket", false);
}

SocketAdapter::~SocketAdapter()
{
	if (_unrealSocket != nullptr)
	{
		_unrealSocket->Close();
	}
}

void SocketAdapter::closeSocket()
{
	if (this != nullptr && _unrealSocket != nullptr)
	{
		_unrealSocket->Close();
	}
}

bool SocketAdapter::checkTwitchConnectionHealth()
{
	if (_unrealSocket != nullptr && _unrealSocket->GetConnectionState() == ESocketConnectionState::SCS_Connected)
	{

		return true;
	}
	else
	{
		return false;
	}
}

int SocketAdapter::receiveMessage(std::string &str)
{
	if (_unrealSocket == nullptr)
	{
		return 0;
	}
	int num = 0;
	TArray<uint8> data_buffer;
	data_buffer.SetNumUninitialized(CONNECTION_BUFFER_SIZE);
	if (_unrealSocket != nullptr)
	{

		_unrealSocket->Recv(data_buffer.GetData(), data_buffer.Num(), num);

		if (num > 0)
		{
			FString f_string_data = "";
			const std::string c_string_data(reinterpret_cast<const char*>(data_buffer.GetData()), num); // Conversion from uint8 to char
			str = c_string_data;
		}
	}
	return num;
}

void SocketAdapter::cleanupSocket()
{
	if (_unrealSocket != NULL)
	{
		_unrealSocket->Close();
	}
	_unrealSocket->Shutdown(ESocketShutdownMode::ReadWrite);
}
bool SocketAdapter::checkInternetConnectionHealth()
{
	if (_socketSubSystem != NULL)
	{
		TSharedPtr<FInternetAddr> remoteAdress = _socketSubSystem->CreateInternetAddr();
		ESocketErrors errors = _socketSubSystem->GetHostByName("www.google.com", *remoteAdress);
		return errors == ESocketErrors::SE_NO_ERROR;
	}
	else
	{
		return false;
	}
}
/*
Sends a message to the server the plugin is connected to at the time of the call
*/
std::string SocketAdapter::sendServerMessageWithResponse(std::string &message)
{
	this->sendServerMessageWithNoResponse(message);
	std::string buffer = "";
	this->receiveMessage(buffer);
	return buffer;
}

void SocketAdapter::sendServerMessageWithNoResponse(std::string &message_val)
{
	int32 bytesSent = 0;
	_unrealSocket->Send((uint8 *)message_val.c_str(), message_val.length(), bytesSent);
}

std::string SocketAdapter::establishServerConnection(const std::string serverAdress, const std::string serverPort)
{

	TSharedPtr<FInternetAddr> remoteAdress = _socketSubSystem->CreateInternetAddr("");
	ESocketErrors errors = _socketSubSystem->GetHostByName(serverAdress.c_str(), *remoteAdress);
	remoteAdress->SetPort(atoi(serverPort.c_str()));
	if (errors != ESocketErrors::SE_NO_ERROR)
	{
		return "Error creating socket " + errors;
	}

	_unrealSocket = _socketSubSystem->CreateSocket(NAME_Stream, "twitchIntegratorSocket", false);
	int newBufferSize = CONNECTION_BUFFER_SIZE;
	_unrealSocket->SetSendBufferSize(CONNECTION_BUFFER_SIZE, newBufferSize);
	_unrealSocket->SetReceiveBufferSize(CONNECTION_BUFFER_SIZE, newBufferSize);
	_unrealSocket->Connect(*remoteAdress);
	return "";
}