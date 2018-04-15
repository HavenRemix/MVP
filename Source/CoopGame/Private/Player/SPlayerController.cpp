// Fill out your copyright notice in the Description page of Project Settings.

#include "SPlayerController.h"

#include "TimerManager.h"
#include "Runtime/InputCore/Classes/InputCoreTypes.h"


// ------- FUNCTIONS ------- \\


void ASPlayerController::RespawnPlayer(float WaitingTime, bool HasToClick)
{
	if (WaitingTime != 0)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_WaitingTime, this, &ASPlayerController::RestartPawn, WaitingTime, false, 1.0f);
		return;
	}
	if (HasToClick)
	{
		RestartPawn();
		return;
	}
}


void ASPlayerController::RestartPawn()
{
	ServerRestartPlayer();
}