// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Engine/EngineTypes.h"
#include "SCharacter.generated.h"

class UCameraComponent;
class ASWeapon;
class USHealthComponent;
class USkeletalMeshComponent;
class AActor;
class USoundCue;
class ASPlayerController;
class USpringArmComponent;
class UAnimSequence;

UCLASS()
class COOPGAME_API ASCharacter : public ACharacter
{
	GENERATED_BODY()

public:

	ASCharacter();

protected:

	virtual void BeginPlay() override;

// ------- INPUT ------- \\

	void MoveForward(float Value);

	void MoveRight(float Value);

	void BeginCrouch();

	void EndCrouch();

	void Zoom();

	void BeginRun();

	void EndRun();

// ------- COMPONENTS ------- \\

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* FPSMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* CameraComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USHealthComponent* HealthComp;

// ------- VARIABLES ------- \\

	float DefaultFOV;

	UPROPERTY(EditDefaultsOnly)
	float ZoomedFOV;

	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = 0.1))
	float ZoomInterpSpeed;

	UPROPERTY(BlueprintReadWrite)
	bool bIsRunning;

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bDied;

// ------- ANIMATIONS ------- \\

	UPROPERTY(EditDefaultsOnly, Category = "Animations")
	UAnimSequence* ReloadAnim;

	UPROPERTY(EditDefaultsOnly, Category = "Animations")
	UAnimSequence* EquipAnim;

	UPROPERTY(EditDefaultsOnly, Category = "Animations")
	UAnimSequence* FPPReloadAnim;

	UPROPERTY(EditDefaultsOnly, Category = "Animations")
	UAnimSequence* FPPEquipAnim;

// ------- FUNCTIONS ------- \\

	void ChangeAnimMode();

// ------- EXTERNALS ------- \\

//Weapon

	int WeaponNum;

	UPROPERTY(VisibleDefaultsOnly, Category = "Weapon")
	FName WeaponAttachSocketName;

	UPROPERTY(VisibleDefaultsOnly, Category = "Weapon")
	FName WeaponBackAttachSocketName;

	void EquipWeapon(TSubclassOf<ASWeapon> Weapon);

	void NextWeaponInput();
	void PreviousWeaponInput();

//Health

	UFUNCTION()
	void OnHealthChanged(USHealthComponent* OwningHealthComp, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

//Timer

	FTimerHandle TimerHandle_ReloadTimer;
	FTimerHandle TimerHandle_FPPReloadTimer;
	FTimerHandle TimerHandle_EquipTimer;
	FTimerHandle TimerHandle_FPPEquipTimer;

public:

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

// ------- EXTERNALS ------- \\

//Weapon

	UPROPERTY(BlueprintReadOnly, Replicated)
	ASWeapon* CurrentWeapon;

	UPROPERTY(BlueprintReadOnly, Replicated)
	ASWeapon* UnequippedWeapon;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TArray<TSubclassOf<ASWeapon>> Weapons;

// ------- FUNCTIONS ------- \\

	virtual FVector GetPawnViewLocation() const override;

	UFUNCTION(BlueprintCallable)
	FRotator GetAimOffsets() const;

	UFUNCTION(BlueprintCallable)
	void StartFire();

	UFUNCTION(BlueprintCallable)
	void StopFire();

	UFUNCTION(BlueprintImplementableEvent)
	void BPReload();

	void Reload();

// ------- AUDIO ------- \\

	UFUNCTION(Exec)
	void PlaySong();

	UPROPERTY(EditDefaultsOnly)
	USoundCue* ReMixTrax;

// ------- VARIABLES ------- \\

	UPROPERTY(BlueprintReadWrite, Category = "Weapons")
	bool bWantsToZoom;
};
