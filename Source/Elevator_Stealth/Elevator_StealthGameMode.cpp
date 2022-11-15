// Copyright Epic Games, Inc. All Rights Reserved.

#include "Elevator_StealthGameMode.h"
#include "Elevator_StealthCharacter.h"
#include "UObject/ConstructorHelpers.h"

AElevator_StealthGameMode::AElevator_StealthGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
