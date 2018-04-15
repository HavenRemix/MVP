// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MenuSystem/MenuInterface.h"
#include "MenuSystem/MainMenu.h"

#include "CoreMinimal.h"
#include "OnlineSessionInterface.h"
#include "OnlineSubsystem.h"
#include "Engine/GameInstance.h"
#include "SGameInstance.generated.h"

class UUserWidget;

/**
 * 
 */
UCLASS()
class COOPGAME_API USGameInstance : public UGameInstance, public IMenuInterface
{
	GENERATED_BODY()

public:

	USGameInstance(const FObjectInitializer & ObjectInitializer);
	
	virtual void Init();

// ------- FUNCTIONS ------- \\

	UFUNCTION(BlueprintCallable)
	void LoadMenuWidget();

	UFUNCTION(BlueprintCallable)
	void InGameLoadMenu();

	UFUNCTION(Exec)
	void Host(FString ServerName) override;

	UFUNCTION(Exec)
	void Join(uint32 Index) override;

	void StartSession();

	virtual void LoadMainMenu() override;

	void RefreshServerList() override;

private:

	TSubclassOf<class UUserWidget> MenuClass;
	TSubclassOf<class UUserWidget> InGameMenuClass;

	class UMainMenu* Menu;

	IOnlineSessionPtr SessionInterface;
	TSharedPtr<class FOnlineSessionSearch> SessionSearch;

	FString DesiredServerName;
	void CreateSession();

// ------- CALLBACK DELEGATES FOR SESSION INTERFACE ------- \\

	void OnCreateSessionComplete(FName SessionName, bool Success);
	void OnDestroySessionComplete(FName SessionName, bool Success);
	void OnFindSessionsComplete(bool Success);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
};
