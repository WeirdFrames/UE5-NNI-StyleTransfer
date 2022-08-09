// Copyright Epic Games, Inc. All Rights Reserved.

#include "Template/NNIStyleTransferTestGameMode.h"
#include "Template/NNIStyleTransferTestCharacter.h"
#include "UObject/ConstructorHelpers.h"

ANNIStyleTransferTestGameMode::ANNIStyleTransferTestGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
