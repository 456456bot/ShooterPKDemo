// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterPKGameMode.h"
#include "ShooterPKCharacter.h"
#include "UObject/ConstructorHelpers.h"

AShooterPKGameMode::AShooterPKGameMode()
	: Super() {
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_Defender"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;
}
