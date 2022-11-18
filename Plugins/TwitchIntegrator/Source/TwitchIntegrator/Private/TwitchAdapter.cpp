// Fill out your copyright notice in the Description page of Project Settings.

#include "TwitchAdapter.h"

#include "Interfaces/IHttpResponse.h"


using namespace std;

mutex _connection_mutex;
TwitchAdapter::TwitchAdapter()
{
	_token = "";
	_username = "";
	_channel = "";
	token_check_interval = TOKEN_CHECK_INTERVAL_DEFAULT;
	commandPrefix = "";
	*twitchConnected = false;
	*twitchConnectionChanged = false;
	*subRecieved = false;
	*subGifted = false;
	*messageWaiting = false;
	*_cheerReceived = false;
	*commandEntered = false;
	*whisperReceived = false;
}

TwitchAdapter::TwitchAdapter(const std::string token, const std::string username, const std::string channel)
{
	_token = token;
	_username = username;
	_channel = channel;
	*twitchConnected = false;
	*twitchConnectionChanged = false;
	*subRecieved = false;
	*subGifted = false;
	*messageWaiting = false;
	*_cheerReceived = false;
	*commandEntered = false;
	*whisperReceived = false;
	token_check_interval = TOKEN_CHECK_INTERVAL_DEFAULT;
}

void TwitchAdapter::sendMessage(const FString message_to_send)
{
	string message_val = "PRIVMSG #" + _channel + " :";
	message_val += TCHAR_TO_UTF8(*message_to_send);
	message_val += "\r\n";
	_socketAdapter.sendServerMessageWithNoResponse(message_val);
}

void TwitchAdapter::Tick()
{
	if (last_token_check == NULL)
	{
		ConnectionAndTokenHealthCheck();
		last_token_check = FDateTime::Now();
	}
	else if (FDateTime::Now().GetSecond() - last_token_check.GetSecond() >= token_check_interval)
	{
		last_token_check = FDateTime::Now();
		ConnectionAndTokenHealthCheck();

		if (*_sessionIsActive && !_socketAdapter.checkInternetConnectionHealth())
		{
			toggleTwitchConnection(false);
			*_sessionIsActive = false;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Connection is still valid to %s"), UTF8_TO_TCHAR(_channel.c_str()));
		}
	}
}


std::string TwitchAdapter::sendConnectionInfo()
{
	int num = 0;
	ULONG size = TwitchAdapter::TEST_CONNECTION_BUFFER_SIZE;
	std::string aouth = "PASS oauth:" + _token + "\r\n";
	std::string nick = "NICK " + _username + "\r\n";
	std::string joinChannel = "JOIN #" + _channel + "\r\n";
	std::string tags = "CAP REQ :twitch.tv/tags\r\n";
	std::string commands = "CAP REQ :twitch.tv/tags twitch.tv/commands\r\n";
	std::string userNotice = "USERNOTICE #" + _channel + "\r\n";

	_socketAdapter.sendServerMessageWithNoResponse(aouth);
	_socketAdapter.sendServerMessageWithNoResponse(nick);
	_socketAdapter.sendServerMessageWithNoResponse(tags);
	_socketAdapter.sendServerMessageWithNoResponse(commands);
	string response = _socketAdapter.sendServerMessageWithResponse(joinChannel);
	return response;
}

void TwitchAdapter::validateConnection()
{

	UE_LOG(LogTemp, Warning, TEXT("about to acquire mutex on channel %s"), UTF8_TO_TCHAR(_channel.c_str()));
	std::lock_guard<std::mutex> lck(_connection_mutex);
	UE_LOG(LogTemp, Warning, TEXT("acquired mutex on channel %s"), UTF8_TO_TCHAR(_channel.c_str()));
	if (!*twitchConnected && !_socketAdapter.checkInternetConnectionHealth())
	{
		*_sessionIsActive = false;
		toggleTwitchConnection(false);
	}
}

void TwitchAdapter::ConnectionAndTokenHealthCheck()
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb("GET");
	HttpRequest->SetHeader("Content-Type", "application/json");

	HttpRequest->SetHeader("Authorization", ("Bearer " + this->_token).data());	

	HttpRequest->SetURL(*FString::Printf(TEXT("%hs"), TWITCH_TOKEN_VALIDATION_URL));


	HttpRequest->OnProcessRequestComplete().BindRaw(this, &TwitchAdapter::onValidateTokenResponse);

	HttpRequest->ProcessRequest();
}

void TwitchAdapter::SendRefreshTokenRequest()
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb("Post");
	HttpRequest->SetHeader("Content-Type", "application/x-www-form-urlencoded");

	HttpRequest->SetURL(*FString::Printf(TEXT("%hs"), TWITCH_TOKEN_VALIDATION_URL));

	HttpRequest->OnProcessRequestComplete().BindRaw(this, &TwitchAdapter::onValidateTokenResponse);

	HttpRequest->ProcessRequest();
}

void TwitchAdapter::onValidateTokenResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	// FString JsonRaw = Response->GetContentAsString();
	// TSharedPtr<FJsonObject> JsonParsed;
	// TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(JsonRaw);
	// if (FJsonSerializer::Deserialize(JsonReader, JsonParsed))
	// {
	// 	FString expiresIn = JsonParsed->GetStringField("expires_in");
	// }
}

std::string TwitchAdapter::connect()
{
	std::string serverConnectionErrorMsg = "";
	*_sessionIsActive = true;
	channelReference = UTF8_TO_TCHAR(("#" + _channel).c_str());
	UE_LOG(LogTemp, Warning, TEXT("About to establish server connection to channel %s"),
	       UTF8_TO_TCHAR(_channel.c_str()));
	serverConnectionErrorMsg = _socketAdapter.establishServerConnection(TWITCH_SERVER_ADDRESS, TWITCH_SERVER_PORT);
	UE_LOG(LogTemp, Warning, TEXT("Established server connection to channel %s"), UTF8_TO_TCHAR(_channel.c_str()));
	if (!serverConnectionErrorMsg.empty() || &_connection_mutex == NULL)
	{
		UE_LOG(LogTemp, Warning, TEXT("Connection failed channel %s"), UTF8_TO_TCHAR(_channel.c_str()));
		toggleTwitchConnection(false);
		return serverConnectionErrorMsg;
	}
	std::string response = sendConnectionInfo();
	UE_LOG(LogTemp, Warning, TEXT("Channel %s connection request Received response %s"),
	       UTF8_TO_TCHAR(_channel.c_str()), UTF8_TO_TCHAR(response.c_str()));
	if ((_messages.Num() > 0 && _messages[0].Contains("NOTICE")) || response.find("NOTICE") != string::npos || response.length() == 0 && (_messages.Num() > 0 && _messages[0].IsEmpty()))
	{
		toggleTwitchConnection(false);
		*_sessionIsActive = false;
		if (_messages.Num() > 0 && _messages[0].Contains("NOTICE"))
		{
			serverConnectionErrorMsg = TCHAR_TO_ANSI(*_messages[0]);
		}
		else if (response.find("NOTICE") != string::npos)
		{
			serverConnectionErrorMsg = response;
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("about to start validating connection for channel %s"),
		       UTF8_TO_TCHAR(_channel.c_str()));
		if (this != NULL)
		{
			_connectionValidationThread = std::thread(&TwitchAdapter::validateConnection, this);
			_connectionValidationThread.detach();
		}
		if (this != NULL)
		{
			UE_LOG(LogTemp, Warning, TEXT("about to start listening to chat on channel %s"),
			       UTF8_TO_TCHAR(_channel.c_str()));
			_chatListeningThread = std::thread(&TwitchAdapter::listenToChat, this);
			_chatMessageParsingThread = std::thread(&TwitchAdapter::ParseReceivedChatMessages, this);
			UE_LOG(LogTemp, Warning, TEXT("started listening to chat on channel %s"), UTF8_TO_TCHAR(_channel.c_str()));
			_chatListeningThread.detach();
			_chatMessageParsingThread.detach();
		}
	}

	return serverConnectionErrorMsg;
}
void TwitchAdapter::addCommand(FString command)
{
	_commands.push_back(command);
}
void TwitchAdapter::removeCommand(FString command)
{
	int commandIndex = -1;
	for (size_t i = 0; i < _commands.size(); i++)
	{
		if (command.Compare(_commands.at(i)) == 0)
		{
			commandIndex = i;
			break;
		}
	}

	if (commandIndex != -1)
	{
		_commands.erase(_commands.begin() + commandIndex);
	}
}

void TwitchAdapter::cleanUp()
{
	*_sessionIsActive = false;
	*twitchConnected = false;
	if (&_chatListeningThread != NULL && _chatListeningThread.joinable())
	{
		_chatListeningThread.join();
	}
	if (&_chatMessageParsingThread != NULL && _chatMessageParsingThread.joinable())
	{
		_chatMessageParsingThread.join();
	}
	_connection_mutex.unlock();
	_socketAdapter.cleanupSocket();
	_commands.clear();
}

void TwitchAdapter::Disconnect() {
	UE_LOG(LogTemp, Warning, TEXT("About to declare session inactive"))
	*_sessionIsActive = false;
	*twitchConnected = false;
	toggleTwitchConnection(*twitchConnected);
	UE_LOG(LogTemp, Warning, TEXT("Toggled connection"))
	if (&_chatListeningThread != NULL && _chatListeningThread.joinable())
	{
		_chatListeningThread.join();
	}
	UE_LOG(LogTemp, Warning, TEXT("Stopped listening to chat"))
	_connection_mutex.unlock();
	UE_LOG(LogTemp, Warning, TEXT("Unlocked connection mutex"))
	_socketAdapter.closeSocket();
	UE_LOG(LogTemp, Warning, TEXT("Closed socket"))
	_socketAdapter.cleanupSocket();
	UE_LOG(LogTemp, Warning, TEXT("Cleaned up socket"))
}


void TwitchAdapter::ParseReceivedChatMessages()
{
	FString currentMessage = "";
	std::string msg = "";
	std::string response = TWITCH_PING_RESPONSE_MESSAGE;
	int num = 1;
	while ( *_sessionIsActive )
	{
		std::lock_guard<std::mutex> MessagesLock(*_dataLock.GetArrayMutex());
		while (messages.Num() > 0)
		{
			_connection_mutex.unlock();
			msg = std::string(TCHAR_TO_UTF8(*messages[0]));
			messages.RemoveAt(0);
			if (msg == TWITCH_PING_MESSAGE)
			{

				_socketAdapter.sendServerMessageWithNoResponse(response);
			}
			else
			{
				FChatMessageData data = parseMessage(msg);
				currentMessage = msg.c_str(); 
				UE_LOG(LogTemp, Log, TEXT("about to parse: %s"), UTF8_TO_TCHAR(msg.c_str()));
				if (!*twitchConnected && currentMessage.Contains(channelReference) && _socketAdapter.checkInternetConnectionHealth())
				{
					toggleTwitchConnection(true);
				}
				else if (currentMessage.StartsWith("@badge") && !currentMessage.Contains("JOIN", ESearchCase::CaseSensitive, ESearchDir::FromStart) && !currentMessage.Contains("USERSTATE", ESearchCase::CaseSensitive, ESearchDir::FromStart))
				{
					currentMessage = msg.c_str();
					_data.Add(data);
					UE_LOG(LogTemp, Log, TEXT("Manipulation: Adding to data with now : %d items"), _data.Num());
					_messages.Add(currentMessage);
					UE_LOG(LogTemp, Log, TEXT("Manipulation: Adding to messages with now : %d items"), _messages.Num());
					*messageWaiting = true;
				}
				else if (!_socketAdapter.checkTwitchConnectionHealth())
				{
					_error_message = currentMessage;
					toggleTwitchConnection(false);
					*_sessionIsActive = false;
					*shouldReconnect = false;
					break;
				}
			}
		}
	}
}

void TwitchAdapter::listenToChat()
{
	std::string msg = "";
	std::string bufferMsg = "";
	FString currentMessage = "";
	FString currentPayload = "";
	FString stringToBeAdded = "";
	int num = 0;
	

		while (*_sessionIsActive)
		{
			while (!currentPayload.Contains("\r\n") && messages.Num() == 0 && *_sessionIsActive)
			{
				bufferMsg = "";
				num = _socketAdapter.receiveMessage(bufferMsg);
				if (num > 0)
				{
					UE_LOG(LogTemp, Log, TEXT("payload received: %s"), UTF8_TO_TCHAR(bufferMsg.c_str()));
					currentPayload.Append(UTF8_TO_TCHAR(bufferMsg.c_str()));
				}
				while (num > 0 && currentPayload.Contains("\r\n"))
				{
					std::lock_guard<std::mutex> MessagesLock(*_dataLock.GetArrayMutex());
					currentPayload.Split("\r\n", &currentPayload, &stringToBeAdded);
					messages.Add(currentPayload);
					if (stringToBeAdded.Len() > 0)
					{
						UE_LOG(LogTemp, Log, TEXT("Message split currentPayload: %s ##### stringToBeAdded: %s"), *currentPayload, *stringToBeAdded);
					}
					currentPayload = stringToBeAdded;
				}
				bufferMsg = "";
			}
		}
		if (*twitchConnected)
		{
			toggleTwitchConnection(false);
		}
		_socketAdapter.closeSocket();
}
FChatMessageData TwitchAdapter::parseMessage(const std::string &msg)
{
	/*String stream to convert values for the _data fields*/
	stringstream ssstream;
	int value = -1;
	smatch sm = smatch();
	FString group = "";
	string value_temp = "";
	bool colorFound = false;
	bool bitsFound = false;
	int red = 0;
	int green = 0;
	int blue = 0;
	string msg_temp (msg); 
	FChatMessageField field_temp;
	FChatMessageData currentData;
	currentData.bits_sent = false;
	currentData.raw_message = UTF8_TO_TCHAR(msg.c_str());
	while (regex_search(msg_temp, sm, _regex_val))
	{
		group = sm[0].str().c_str();
		if (group.Contains(";bits") && !bitsFound)
		{
			*_cheerReceived = true;
			currentData.bits_sent = true;
			value_temp = sm[2].str();
			ssstream << value_temp;
			ssstream >> currentData.number_of_bits;
			ssstream.str("");
			ssstream.clear();
		}
		else if (group.Contains("subscriber"))
		{
			value_temp = sm[2].str();
			ssstream << value_temp;
			ssstream >> value;
			currentData.sender_is_subbed = value > 0;
			ssstream.str("");
			ssstream.clear();
		}
		else if (group.Contains("display-name"))
		{
			value_temp = sm[2].str();
			currentData.sender_username = value_temp.c_str();
			currentData.reciever_username = value_temp.c_str();
		}
		else if (group.Contains("color") && sm[2].str().length() >= 7)
		{
			colorFound = true;
			value_temp = sm[2].str();
			vector<string> str_values(3);
			str_values[0] = value_temp.substr(1, 2);
			str_values[1] = value_temp.substr(3, 2);
			str_values[2] = value_temp.substr(5, 2);

			ssstream << hex << str_values[0];
			ssstream >> red;
			ssstream.str("");
			ssstream.clear();
			ssstream << hex << str_values[1];
			ssstream >> green;
			ssstream.str("");
			ssstream.clear();
			ssstream << hex << str_values[2];
			ssstream >> blue;
			ssstream.str("");
			ssstream.clear();

			FColor color(red, green, blue, TwitchAdapter::ALPHA_CHAT_COLOR);
			currentData.sender_username_color_byte = color;
			currentData.sender_username_color = FLinearColor(color);
		}
		else if (group.Contains("mod="))
		{
			value_temp = sm[2].str();
			ssstream << value_temp;
			ssstream >> value;
			currentData.isModerator = value > 0;
			ssstream.str("");
			ssstream.clear();
		}
		else if (group.Contains("badge"))
		{
			currentData.isVIP = group.Contains("vip");
			ssstream.str("");
			ssstream.clear();
		}
		else if (group.Contains("msg-id"))
		{
			if (group.Contains("sub"))
			{
				*subRecieved = true;
				if (group.Contains("gift"))
				{
					*subGifted = true;
					currentData.is_gift_sub = true;
				}
			}
			else if (group.Contains("highlighted-message"))
			{
				currentData.is_highlight_message = true;
			}
			value_temp = sm[2].str();
			currentData.notice_message = value_temp.c_str();
			ssstream.str("");
			ssstream.clear();
		}
		else if (group.Contains("msg-param-cumulative-months") || group.Contains("msg-param-months") || group.Contains("msg-param-streak-months"))
		{
			value_temp = sm[2].str();
			ssstream << value_temp;
			ssstream >> value;
			currentData.sub_month_total = value;
			ssstream.str("");
			ssstream.clear();
		}
		else if (group.Contains("msg-param-recipient-display-name"))
		{
			value_temp = sm[2].str();
			currentData.reciever_username = value_temp.c_str();
		}
		else if (group.Contains("ms-param-sender-name"))
		{
			value_temp = sm[2].str();
			currentData.gifter_username = value_temp.c_str();
		}
		else
		{
			value_temp = sm[1].str();
			field_temp.field_name = value_temp.c_str();
			value_temp = sm[2].str();
			field_temp.field_value = value_temp.c_str();
			currentData.other_Data.Add(field_temp);
		}
		msg_temp = sm.suffix().str();
	}
	if (currentData.sender_username.IsEmpty())
	{
		regex_search(msg, sm, _regex_val_username_fallback);
		value_temp = sm[1].str();
		currentData.sender_username = UTF8_TO_TCHAR(value_temp.c_str());
	}
	if (regex_search(msg, sm, _regex_val_message) && sm[1].str().length() > 0)
	{
		currentData.message = UTF8_TO_TCHAR(sm[1].str().c_str());
		currentData.messsageText = FText::FromString(currentData.message);
		currentData.isWhisper = false;
		*whisperReceived = false;
	}
	else if (regex_search(msg, sm, _regex_val_message_whisper))
	{
		currentData.message = UTF8_TO_TCHAR(sm[1].str().c_str());
		currentData.messsageText = FText::FromString(currentData.message);
		currentData.isWhisper = true;
		*whisperReceived = true;
	}
	for (size_t i = 0; i < _commands.size(); i++)
	{
		if (currentData.message.Contains(UTF8_TO_TCHAR(commandPrefix.c_str()) + _commands.at(i)))
		{
			currentData.commands_entered.Add(_commands.at(i));
			currentData.containsCommands = true;
			*commandEntered = true;
		}
	}
	if (!colorFound)
	{
		srand(time(NULL));
		red = rand() % TwitchAdapter::BYTE_COLOR_MAX_VALUE;
		green = rand() % TwitchAdapter::BYTE_COLOR_MAX_VALUE;
		blue = rand() % TwitchAdapter::BYTE_COLOR_MAX_VALUE;
		FColor color(red, green, blue, TwitchAdapter::ALPHA_CHAT_COLOR);
		currentData.sender_username_color_byte = color;
		currentData.sender_username_color = FLinearColor(color);
	}
	else
	{
		currentData.colorIsAccurate = true;
	}
	return currentData;
}

void TwitchAdapter::sendWhisper(const FString message_to_send, const FString receiver)
{
	this->sendMessage("/w " + receiver + " " + message_to_send);
}


void TwitchAdapter::toggleTwitchConnection(const bool state)
{
	*twitchConnectionChanged = true;
	*twitchConnected = state;
}
