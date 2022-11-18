// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Misc/ScopeLock.h"
#include <string>
#include <regex>
#include "HttpModule.h"
#include "GenericPlatform/HttpRequestImpl.h"
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <thread>
#include <mutex>
#include "Windows/AllowWindowsPlatformTypes.h"
#include "SocketAdapter.h"
#include "TwitchStructs.h"
#include "TwitchIntegrator/DataLock.h"
#include "Windows/HideWindowsPlatformTypes.h"


/**
 * 
 */
class TWITCHINTEGRATOR_API TwitchAdapter
{
public:


	TwitchAdapter();

	TwitchAdapter(const std::string token,const std::string username,const std::string channel);

	void sendMessage(const FString message_to_send);

	void sendWhisper(const FString message_to_send, const FString receiver);

	bool* messageWaiting = new bool;

	bool* _cheerReceived = new bool;

	bool* commandEntered = new bool;

	bool* twitchConnected = new bool;

	bool* twitchConnectionChanged = new bool;

	bool* whisperReceived = new bool;

	bool* subRecieved = new bool;

	bool* subGifted = new bool;
	bool* shouldReconnect = new bool;

	DataLock _dataLock; 
	
	/*Raw message sent by the Twitch API*/
	TArray<FString> _messages;

	FString _error_message;

	TArray<FChatMessageData> _data;

	std::string commandPrefix;

	std::string connect();

	void addCommand(FString command);

	void removeCommand(FString command);

	void cleanUp();

	void Disconnect();

	void Tick();

	/*The alpha value of the color of */
	static const int ALPHA_CHAT_COLOR = 255;

	/*The interval at which we check for the validity of the token*/
	static const time_t TOKEN_CHECK_INTERVAL = 60;

	/*The upper limit of each channel value to create a color*/
	static const int BYTE_COLOR_MAX_VALUE = BYTE_MAX + 1;
	
	bool* _sessionIsActive = new bool;


private:
	std::string _token;
	
	std::string _username;
	
	std::string _channel;
	
	std::vector<FString> _commands;

	/*Time of the last token check*/
	FDateTime last_token_check;

	/*Inteval at wich we check the token expiration*/
	int token_check_interval;

	/*The number of the port of the irc twitch server*/
	const char * TWITCH_SERVER_PORT = static_cast<const char *>("6667");

	/*Address used to connect to the twitch IRC server*/
	const char * TWITCH_SERVER_ADDRESS = static_cast<const char *>("irc.chat.twitch.tv");

	/*The size of the buffer used during the connection to chat*/
	static const int32 TEST_CONNECTION_BUFFER_SIZE = 2048;

	/*The interval at wich the token expiration is checked*/
	static const int TOKEN_CHECK_INTERVAL_DEFAULT = 10;

	/*The string sent by twitch when it wants to check if the connection is still active*/
	const char * TWITCH_PING_MESSAGE = static_cast<const char *>("PING :tmi.twitch.tv");

	/*The string sent back to twitch to tell it that the connection is active*/
	const char * TWITCH_PING_RESPONSE_MESSAGE = static_cast<const char *>("PONG :tmi.twitch.tv\r\n");
	/*The string sent by twitch when it wants to check if the connection is still active*/
	const char * TWITCH_MODERATOR_BADGE_VALUE = "1";
	/*The url used to validate the token*/
	const char * TWITCH_TOKEN_VALIDATION_URL = "https://id.twitch.tv/oauth2/validate";
	/*The twitch url to do token operations*
	const char * TWITCH_TOKEN_VALIDATION_URL = "https://id.twitch.tv/oauth2/token";
	/*Regex pattern to capture all metadata from the chat message*/
	 char * _regex_str = "[@;]([a-z\\-]+)+=([A-z0-9\\-#/,]+)+(?=.* :)";
	/*Regex pattern to capture the actual chat message without metadata*/
	char * _regex_str_message = "(?:#\\w+) :(.*)";
	/*Regex pattern to capture the username in case the twitch API doesn't put (it happened in some weird cases)*/
	char *  _regex_str_username_fallback = "^@.*:(\\w+)!.*(?:#\\w+ :)";
	/*Regex pattern to capture the actual message of a whisper*/
	char * _regex_str_whisper_message = "(?:WHISPER \\w+) :(.*)";
	/*Regex to capture all metadata from the chat message*/
	std::regex  _regex_val = std::regex(static_cast<const std::string>("[@;]([a-z\\-]+)=([\\w\\-#/,]+)(?=.* :)"));
	/*regex to capture the acutal chat message without metatdata*/
	std::regex _regex_val_message = std::regex(static_cast<const std::string>(_regex_str_message));
	/**Regex pattern to capture the username in case the twitch API doesn't put (it happened in some weird cases)*/
	std::regex _regex_val_username_fallback = std::regex(static_cast<const std::string>(_regex_str_username_fallback));
	/*Regex pattern to capture the actual message when the message is a whisper*/
	std::regex _regex_val_message_whisper = std::regex(static_cast<const std::string>(_regex_str_whisper_message));

	SocketAdapter _socketAdapter;

	std::thread _chatListeningThread;

	std::thread _connectionValidationThread;

	std::thread _chatMessageParsingThread;

	FString channelReference;

	TArray<FString> messages;

	std::string sendConnectionInfo();

	void listenToChat();

	FChatMessageData parseMessage(const std::string &msg);

	void toggleTwitchConnection(const bool state);

	void validateConnection();

	void ConnectionAndTokenHealthCheck();
	void SendRefreshTokenRequest();

	void onValidateTokenResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	
	void ParseReceivedChatMessages();
};
