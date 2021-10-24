// Copyright Epic Games, Inc. All Rights Reserved.

#include "RebellionGameMode.h"
#include "RebellionCharacter.h"
#include "UObject/ConstructorHelpers.h"

ARebellionGameMode::ARebellionGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
