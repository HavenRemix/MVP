// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SWeapon.generated.h"


// Contains information of a single hitscan weapon linetrace
USTRUCT()
struct FHitScanTrace
{
	GENERATED_BODY()

public:

	UPROPERTY()
	TEnumAsByte<EPhysicalSurface> SurfaceType;

	UPROPERTY()
	FVector_NetQuantize TraceTo;
};


UCLASS()
class COOPGAME_API ASWeapon : public AActor
{
	GENERATED_BODY()

public:

	ASWeapon();

	virtual void StartFire();
	virtual void StopFire();

	virtual void ReloadWeapon();

// ------- EXTRA ------- \\

	UFUNCTION()
	virtual bool IsTargeting(bool WasTargeting);

	UPROPERTY(EditDefaultsOnly)
	bool IsRifle;

protected:

	virtual void BeginPlay() override;

// ------- COMPONENTS ------- \\

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USkeletalMeshComponent* MeshComp;

// ------- VARIBLES ------- \\

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Trinity")
	FName MuzzleSocketName;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Trinity")
	FName TracerTargetName;

	UPROPERTY(EditDefaultsOnly)
	bool bIsProjectile;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int CurrentAmmo;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int MaxAmmo;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int ClipSize;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float BaseDamage;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float RateOfFire;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile", meta = (ClampMin = 0.0f))
	float AimBulletSpread;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile", meta = (ClampMin = 0.0f))
	float DefaultBulletSpread;

	float BulletSpread;

	float LastFireTime;

	float TimeBetweenShots;

// ------- FUNCTIONS ------- \\

	void PlayFireEffects(FVector TraceEnd);

	void PlayImpactEffects(EPhysicalSurface SurfaceType, FVector ImpactPoint);

	void Fire();

	void Reload();

	UFUNCTION(BlueprintImplementableEvent)
	void SpawnProjectile();

// ------- SERVER FUNCTIONS ------- \\

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFire();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerReload();

	UFUNCTION()
	void OnRep_HitScanTrace();

// ------- AUDIO ------- \\

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	class USoundCue* FireStartSound;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	class USoundCue* FireEndSound;

// ------- REFERENCES ------- \\

	UPROPERTY(ReplicatedUsing = OnRep_HitScanTrace)
	FHitScanTrace HitScanTrace;

	UPROPERTY(EditDefaultsOnly, Category = "Trinity")
	TSubclassOf<class UCameraShake> FireCamShake;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trinity")
	TSubclassOf<UDamageType> DamageType;

	FTimerHandle TimerHandle_TimeBetweenShots;

// ------- EMMITERS ------- \\

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trinity")
	class UParticleSystem* MuzzleEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trinity")
	class UParticleSystem* DefaultImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trinity")
	class UParticleSystem* FleshImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trinity")
	class UParticleSystem* TracerEffect;
};
