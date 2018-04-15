// Fill out your copyright notice in the Description page of Project Settings.

#include "GameMode/Lobby/LobbyDeathmatchGameMode.h"
#include "Engine/World.h"
#include "TimerManager.h"


void ALobbyDeathmatchGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	++NumberOfPlayers;

	if (NumberOfPlayers >= 8)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_Countdown, this, &ALobbyDeathmatchGameMode::JoinPlayers, 5.0f);
	}
}


void ALobbyDeathmatchGameMode::Logout(AController* Existing)
{
	Super::Logout(Existing);

	--NumberOfPlayers;
}


void ALobbyDeathmatchGameMode::JoinPlayers()
{
	UWorld* World = GetWorld();
	if (!ensure(World != nullptr)) return;

	bUseSeamlessTravel = true;
	World->ServerTravel("/Game/Maps/Deathmatch/DM_SwitchArea17");
}
