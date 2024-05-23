// Copyright Epic Games, Inc. All Rights Reserved.

#include "InteractGrabCppGameMode.h"
#include "InteractGrabCppCharacter.h"
#include "UObject/ConstructorHelpers.h"

AInteractGrabCppGameMode::AInteractGrabCppGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
