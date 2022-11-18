/*

Copyright (c) 2017-2018 Louis Contant louiscontant@hotmail.com



This file is part of the project Twitch Integrator

Twitch Integrator can not be copied and/or distributed without the express
persmission of Louis Contant


*/

#include "twitchChat.h"

using namespace std;

// Sets default values
AtwitchChat::AtwitchChat()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1 / 60;
	bIsConnected = false;
}

/*
 Handles the connection routine to a twitch chat
*/

/*
Does what it says, sends a message to the designated chat
*/
void AtwitchChat::sendMessage(const FString message_to_send)
{
	_twitchAdapter.sendMessage(message_to_send);
}

void AtwitchChat::sendWhisper(const FString message_to_send, const FString receiver)
{
	_twitchAdapter.sendWhisper(message_to_send, receiver);
}

void AtwitchChat::setUsername(const FString username_param)
{
	usernameInitialized = true;
	_username_str = TCHAR_TO_ANSI(*username_param);
}

void AtwitchChat::setToken(const FString token_param)
{
	tokenInitialized = true;
	_token_str = TCHAR_TO_ANSI(*token_param);
}

void AtwitchChat::setChannel(const FString channel_param)
{
	channelInitialized = true;
	_channel_str = TCHAR_TO_ANSI(*channel_param.ToLower());
}

void AtwitchChat::addCommand(const FString command)
{
	_twitchAdapter.addCommand(command);
}

void AtwitchChat::removeCommand(const FString command)
{
	_twitchAdapter.removeCommand(command);
}

void AtwitchChat::disconnectFromTwitch()
{
	_twitchAdapter.Disconnect();
}

void AtwitchChat::setCommandPrefix(const FString command_prefix)
{
	commandPrefix = command_prefix;
	_command_prefix_str = TCHAR_TO_ANSI(*command_prefix);
	_twitchAdapter.commandPrefix = _command_prefix_str;
}

/**
A loop that will continusly listen to the designated chat and fill the various info for the events
These events will be triggered in the Click function

**/

/**
* Estblishes the connection to the designated Twitch chat
**/
void AtwitchChat::connectToTwitch()
{
	UE_LOG(LogTemp, Warning, TEXT("About to connect to channel %s was called"), UTF8_TO_TCHAR(_channel_str.c_str()));
	if (tokenInitialized && channelInitialized && usernameInitialized && !*_twitchAdapter.twitchConnected)
	{
		UE_LOG(LogTemp, Warning, TEXT("Variables valid state on channel %s"), UTF8_TO_TCHAR(_channel_str.c_str()));
		_twitchAdapter = TwitchAdapter(_token_str, _username_str, _channel_str);
		UE_LOG(LogTemp, Warning, TEXT("Created new twitch adapter to channel %s"), UTF8_TO_TCHAR(_channel_str.c_str()));
		_twitchAdapter.commandPrefix = _command_prefix_str;
		UE_LOG(LogTemp, Warning, TEXT("About to connect to channel %s"), UTF8_TO_TCHAR(_channel_str.c_str()));
		std::string error = _twitchAdapter.connect();
		if (!error.empty() && bEnableDebugMessages)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, UTF8_TO_TCHAR(error.c_str()));
		}
	}
}

// Called when the game starts or when spawned
void AtwitchChat::BeginPlay()
{
	AActor::BeginPlay();

	tokenInitialized = false;
	channelInitialized = false;
	usernameInitialized = false;
	_actorEndingPlay = false;
	_token_str = "";
	_username_str = "";
	_channel_str = "";
	if (TCHAR_TO_ANSI(*token))
	{
		_token_str = TCHAR_TO_ANSI(*token);
		tokenInitialized = !_token_str.empty();
	}
	if (TCHAR_TO_ANSI(*channel))
	{
		_channel_str = TCHAR_TO_ANSI(*channel.ToLower());
		channelInitialized = !_channel_str.empty();
	}
	if (TCHAR_TO_ANSI(*username))
	{
		_username_str = TCHAR_TO_ANSI(*username);
		usernameInitialized = !_username_str.empty();
	}

	if (TCHAR_TO_ANSI(*commandPrefix))
	{
		_command_prefix_str = TCHAR_TO_ANSI(*commandPrefix);
	}
	else
	{
		_command_prefix_str = "!";
	}

	if (tokenInitialized && channelInitialized && usernameInitialized)
	{
		connectToTwitch();
	}
}

/*
Using this to clean up behind myself
*/
void AtwitchChat::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (*_twitchAdapter._sessionIsActive)
	{
		_twitchAdapter.cleanUp();
	}
	AActor::EndPlay(EndPlayReason);
}

// Called every frame
void AtwitchChat::Tick(float DeltaTime)
{
	AActor::Tick(DeltaTime);
	/*message waiting is modified in listenChat() if there's a message that's received from the twitch API*/
	if (_twitchAdapter._messages.Num() > 0 && _twitchAdapter._data.Num() > 0)
	{
		FString currentMessage = _twitchAdapter._messages[0];
		FChatMessageData messageData = _twitchAdapter._data[0];
		//TODO: Try to just use the raw message in _twitchAdapter._data instead to see if it helps with null pointers
		if (!currentMessage.Contains("USERSTATE") && !currentMessage.Contains("JOIN"))
		{
			OnChatMessage.Broadcast(currentMessage, messageData);
			UE_LOG(LogTemp, Log, TEXT("Manipulation: Triggered message event with %d messages left and %d message data left"),
			       _twitchAdapter._messages.Num(), _twitchAdapter._data.Num());
		}
		/*cheerReceived is modified in listenChat() if there's a cheer in a message*/
		if (messageData.bits_sent && !currentMessage.IsEmpty())
		{
			onCheer.Broadcast(messageData.sender_username, messageData.message, messageData.number_of_bits);
		}
		if (messageData.containsCommands && !currentMessage.IsEmpty())
		{
			onChatCommandEntered.Broadcast(currentMessage, messageData);
			*_twitchAdapter.commandEntered = false;
		}

		if (messageData.isWhisper && !currentMessage.IsEmpty())
		{
			onWhisper.Broadcast(currentMessage, messageData);
			*_twitchAdapter.whisperReceived = false;
		}
		if (messageData.is_sub_event && !currentMessage.IsEmpty())
		{
			onChatSubRecieved.Broadcast(messageData.reciever_username, messageData.messsageText, currentMessage,
			                            messageData.sub_month_total, messageData.gifter_username);
			*_twitchAdapter.subRecieved = false;
			*_twitchAdapter.subGifted = false;
		}
		_twitchAdapter._messages.Remove(currentMessage);
		UE_LOG(LogTemp, Log, TEXT("Manipulation: removed message with %d messages left"),
			       _twitchAdapter._messages.Num());
		_twitchAdapter._data.Remove(messageData);
		UE_LOG(LogTemp, Log, TEXT("Manipulation: removed data with %d data left"),
			       _twitchAdapter._data.Num());
	}
	if (*_twitchAdapter.twitchConnectionChanged)
	{
		bIsConnected = *_twitchAdapter.twitchConnected;
		if (!*_twitchAdapter.twitchConnected)
		{
			if (bEnableDebugMessages)
			{
				GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, _twitchAdapter._error_message);
			}
		}
		onConnectionChanged.Broadcast(*_twitchAdapter.twitchConnected);
		*_twitchAdapter.twitchConnectionChanged = false;
		if (*_twitchAdapter.shouldReconnect && !*_twitchAdapter.twitchConnected)
		{
			connectToTwitch();
		}
	}


	_twitchAdapter.Tick();
}
