// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "TwitchStructs.generated.h"
/*
Data struct for any metadata field that don't have any specific use they are put in an array called other
*/
USTRUCT(BlueprintType)
struct TWITCHINTEGRATOR_API FChatMessageField {
	GENERATED_BODY()
	/*Name of the metadata field*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Twitch Integrator")
		FString field_name;
	/*Value of the metadata field*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Twitch Integrator")
		FString field_value;
};

/*
Data struct for all the main chat message metadata;
*/
USTRUCT(BlueprintType)
struct TWITCHINTEGRATOR_API FChatMessageData {
	GENERATED_BODY()

	/*The username of the person that send the message*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Twitch Integrator")
		FString sender_username;
	/*If the sender of the message is subbed to the channel*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Twitch Integrator")
		bool sender_is_subbed;
	/*If there were bits send with the message*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Twitch Integrator")
		bool bits_sent;
	/*The ammount of bits sent with the message*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Twitch Integrator")
		float number_of_bits;
	/*The message without metadata*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Twitch Integrator")
		FString message;
	/*The message with metadat*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Twitch Integrator")
		FString raw_message;
	/*The message without metadata in TEXT form*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Twitch Integrator")
		FText messsageText;
	/*The color of the sender's username in Linear Color*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Twitch Integrator")
		FLinearColor sender_username_color;
	/*Sometimes, the field color of the sender's username is empty. In that situation, I generate one at random
	if I did so, this value will equal True*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Twitch Integrator")
		bool colorIsAccurate;
	/*The color of the sender's username in Color form*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Twitch Integrator")
		FColor sender_username_color_byte;
	/*If the user is a moderator for the selected channel*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Twitch Integrator")
		bool isModerator;
	/*Other meta data fields*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Twitch Integrator")
		TArray<FChatMessageField> other_Data;
	/*List of all the commands entered*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Twitch Integrator")
		TArray<FString> commands_entered;
	/*Weither or not the message contains one or more commands*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Twitch Integrator")
		bool containsCommands;
	/*If the sender is VIP*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Twitch Integrator")
		bool isVIP;
	/*If the message is a whisper*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Twitch Integrator")
		bool isWhisper;
	/*Total number of months the user has been subbed for*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Twitch Integrator")
		float sub_month_total;
	/*If the subscription is event is a gift sub*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Twitch Integrator")
		bool is_gift_sub;
	/*If the subscription is event is a gift sub*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Twitch Integrator")
		bool is_sub_event;
	/*If the subscription is event is a gift sub*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Twitch Integrator")
		bool is_highlight_message;
	/*The name of the user that gifted the sub(s)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Twitch Integrator")
		FString gifter_username;
	/*The name of the user that gifted the sub(s)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Twitch Integrator")
		FString reciever_username;
	/*The tier of the sub (0: prime, 1: 5$, 2: 10$ 3: 25$)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Twitch Integrator")
		float sub_Tier;
	/*The name of the tier of the sub*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Twitch Integrator")
		float sub_plan_name;
	/*The number of months gifted as part of a single sub gift*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Twitch Integrator")
		float months_gifted;
	/*The message along side the event like "User highlighted the message"*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Twitch Integrator")
		FString notice_message;
	/**
		 * Lexicographically test whether the left data is == the right data 
		 *
		 * @param Lhs chat data to compare against.
		 * @param Rhs chat data to compare against.
		 * @return true if the left data is lexicographically == the data string, otherwise false
		 */
	UE_NODISCARD FORCEINLINE friend bool operator==(const FChatMessageData& data1, const FChatMessageData& data2)
	{
		//Compares all the fields of data1 with all the field of data2 to check if they're == 
		return data1.raw_message == data2.raw_message;
	}

};


/*Event triggered when receiveign a message from the Twitch API*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FChatMessageReceived, FString, message, FChatMessageData, data);
/*Event triggered when receiving a whisper*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FChatWhipserReceived, FString, message, FChatMessageData, data);
/*Event triggered when a command that was previously set is entered by any user*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FChatCommandEntered, FString, message, FChatMessageData, data);
/*Event triggered when receiveign a message with cheers from the Twitch API*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FChatCheerReceived, FString, sender_username, FString, message, int32, ammount);
/*Event triggered when the connection to twitch chat changes*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTwitchConnectionChanged, bool, connectionStatus);
/*Event triggered when someone shares their sub, their resub or recieves a gifted sub*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FChatSubReceived, FString, sub_username, FText, message,FString, raw_message ,int32, streak, FString, gifter_username);