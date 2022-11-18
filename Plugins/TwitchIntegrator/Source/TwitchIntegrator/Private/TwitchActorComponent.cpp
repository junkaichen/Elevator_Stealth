// Fill out your copyright notice in the Description page of Project Settings.

#include "TwitchActorComponent.h"



// Sets default values for this component's properties
UTwitchActorComponent::UTwitchActorComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 1/60;
	bIsConnected = false;
	// ...
}


// Called when the game starts
void UTwitchActorComponent::BeginPlay()
{
	Super::BeginPlay();

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
	if (TCHAR_TO_ANSI(*username)) {
		_username_str = TCHAR_TO_ANSI(*username);
		usernameInitialized = !_username_str.empty();

	}
		
	if (TCHAR_TO_ANSI(*commandPrefix)) {
		_command_prefix_str = TCHAR_TO_ANSI(*commandPrefix);
	}
	else {
		_command_prefix_str = "!";
	}

	if (tokenInitialized && channelInitialized && usernameInitialized) {
		connectToTwitch();
	}
	// ...

}


// Called every frame
void UTwitchActorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	std::lock_guard<std::mutex> lck(*_twitchAdapter._dataLock.GetArrayMutex());
	/*message waiting is modified in listenChat() if there's a message that's received from the twitch API*/
	if (_twitchAdapter._data.Num() > 0)
	{
		FString currentMessage = _twitchAdapter._data[0].raw_message;
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

void UTwitchActorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (_twitchAdapter._sessionIsActive) {
		_twitchAdapter.cleanUp();
	}

}

void UTwitchActorComponent::OnComponentDestroyed(const bool bDestroyingHierarchy)
{
	if (bDestroyingHierarchy && _twitchAdapter._sessionIsActive) {
		_twitchAdapter.cleanUp();
	}

}

void UTwitchActorComponent::sendMessage(const FString message_to_send)
{
	_twitchAdapter.sendMessage(message_to_send);
}

void UTwitchActorComponent::setUsername(const FString username_param)
{
	usernameInitialized = true;
	_username_str = TCHAR_TO_ANSI(*username_param);
}

void UTwitchActorComponent::setToken(const FString token_param)
{
	tokenInitialized = true;
	_token_str = TCHAR_TO_ANSI(*token_param);
}

void UTwitchActorComponent::setChannel(const FString channel_param)
{
	channelInitialized = true;
	_channel_str = TCHAR_TO_ANSI(*channel_param);
}

void UTwitchActorComponent::setCommandPrefix(const FString command_prefix)
{
	commandPrefix = command_prefix;
	_command_prefix_str = TCHAR_TO_ANSI(*command_prefix);
	_twitchAdapter.commandPrefix = _command_prefix_str;
}

void UTwitchActorComponent::addCommand(const FString command)
{
	_twitchAdapter.addCommand(command);
}

void UTwitchActorComponent::removeCommand(const FString command)
{
	_twitchAdapter.removeCommand(command);
}

void UTwitchActorComponent::sendWhisper(const FString message, const FString receiver)
{
	_twitchAdapter.sendWhisper(message, receiver);
}

void UTwitchActorComponent::connectToTwitch()
{
	if (tokenInitialized && channelInitialized && usernameInitialized && !*_twitchAdapter.twitchConnected) {
		_twitchAdapter = TwitchAdapter(_token_str, _username_str, _channel_str);
		_twitchAdapter.commandPrefix = _command_prefix_str;
		std::string error = _twitchAdapter.connect();
		if (!error.empty() && bEnableDebugMessages) {
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, error.c_str());
		}
	}
}

void UTwitchActorComponent::disconnectFromTwitch()
{
	_twitchAdapter.Disconnect();
}
