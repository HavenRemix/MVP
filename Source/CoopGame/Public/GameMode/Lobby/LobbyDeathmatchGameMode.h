// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameMode/DeathmatchGameMode.h"
#include "LobbyDeathmatchGameMode.generated.h"

/**
 * 
 */
UCLASS()
class COOPGAME_API ALobbyDeathmatchGameMode : public ADeathmatchGameMode
{
	GENERATED_BODY()
	
public:

	void PostLogin(APlayerController* NewPlayer) override;

	void Logout(AController* Existing) override;
	
	void JoinPlayers();

private:

	uint32 NumberOfPlayers = 0;

	FTimerHandle TimerHandle_Countdown;
	
};
