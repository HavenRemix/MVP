// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class COOPGAME_API ASPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:


// ------- VARIABLES ------- \\


	FTimerHandle TimerHandle_WaitingTime;


// ------- FUNCTIONS ------- \\


	UFUNCTION()
	void RestartPawn();


public:

	UFUNCTION()
	void RespawnPlayer(float WaitingTime, bool HasToClick);
	
	
};
