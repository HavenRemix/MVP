// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/SWeapon.h"
#include "Player/SCharacter.h"

#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "CoopGame.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundCue.h"
#include "GameFramework/Character.h"


static int32 DebugWeaponDrawing = 0;
FAutoConsoleVariableRef CVARDebugWeaponDrawing(
	TEXT("COOP.DebugWeapons"),
	DebugWeaponDrawing,
	TEXT("Draw Debug Lines for Weapons"),
	ECVF_Cheat);


ASWeapon::ASWeapon()
{
	//Defaults

	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	MuzzleSocketName = "MuzzleSocket";
	TracerTargetName = "Target";

	CurrentAmmo = 60;
	MaxAmmo = 540;
	ClipSize = 60;

	BaseDamage = 20.0f;
	DefaultBulletSpread = 2.0f;
	AimBulletSpread = 0.0f;
	BulletSpread = DefaultBulletSpread;

	RateOfFire = 600;

	SetReplicates(true);

	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;

	bIsProjectile = false;
}


void ASWeapon::BeginPlay()
{
	Super::BeginPlay();

	TimeBetweenShots = 60 / RateOfFire;
}


// ------- FUNCTIONS ------- \\


void ASWeapon::Fire()
{
	// Trace the world, from pawn eyes to crosshair location

	if (Role < ROLE_Authority)
	{
		ServerFire();
	}

	if (!bIsProjectile)
	{
		auto MyOwner = GetOwner();
		if (MyOwner)
		{
			//Deduce Ammo
			if (CurrentAmmo > 0)
			{
				CurrentAmmo--;
			}
			else {
				return;
			}

			//Play Start Sound
			UGameplayStatics::PlaySoundAtLocation(this, FireStartSound, GetActorLocation());

			FVector EyeLocation;
			FRotator EyeRotation;
			MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

			FVector ShotDirection = EyeRotation.Vector();

			// Bullet Spread
			float HalfRad = FMath::DegreesToRadians(BulletSpread);
			ShotDirection = FMath::VRandCone(ShotDirection, HalfRad, HalfRad);

			FVector TraceEnd = EyeLocation + (ShotDirection * 10000);

			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(MyOwner);
			QueryParams.AddIgnoredActor(this);
			QueryParams.bTraceComplex = true;
			QueryParams.bReturnPhysicalMaterial = true;

			// Particle "Target" parameter
			FVector TracerEndPoint = TraceEnd;

			EPhysicalSurface SurfaceType = SurfaceType_Default;

			FHitResult Hit;
			if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, TraceEnd, COLLISION_WEAPON, QueryParams))
			{
				// Blocking hit! Process damage
				AActor* HitActor = Hit.GetActor();

				SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());

				float ActualDamage = BaseDamage;
				if (SurfaceType == SURFACE_FLESHVULNERABLE)
				{
					ActualDamage *= 4.0f;
				}

				UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, ShotDirection, Hit, MyOwner->GetInstigatorController(), MyOwner, DamageType);

				PlayImpactEffects(SurfaceType, Hit.ImpactPoint);

				TracerEndPoint = Hit.ImpactPoint;

			}

			if (DebugWeaponDrawing > 0)
			{
				DrawDebugLine(GetWorld(), EyeLocation, TraceEnd, FColor::White, false, 1.0f, 0, 1.0f);
			}

			PlayFireEffects(TracerEndPoint);

			if (Role == ROLE_Authority)
			{
				HitScanTrace.TraceTo = TracerEndPoint;
				HitScanTrace.SurfaceType = SurfaceType;
			}

			LastFireTime = GetWorld()->TimeSeconds;

			//Play End Sound
			UGameplayStatics::PlaySoundAtLocation(this, FireEndSound, GetActorLocation());
		}
	}
	else {
		AActor* MyOwner = GetOwner();
		if (MyOwner)
		{
			//Deduce Ammo
			if (CurrentAmmo > 0)
			{
				CurrentAmmo--;
			}
			else {
				return;
			}

			//Play Sound
			UGameplayStatics::PlaySoundAtLocation(this, FireStartSound, GetActorLocation());

			//Run Function To Shoot
			SpawnProjectile();
		}
	}
}


void ASWeapon::Reload()
{
	if (Role < ROLE_Authority)
	{
		ServerReload();
	}

	int Temp;
	if (MaxAmmo <= 0)
	{
		return;
	}
	if (MaxAmmo >= ClipSize)
	{
		Temp = ClipSize - CurrentAmmo;
		CurrentAmmo = ClipSize;
		MaxAmmo = MaxAmmo - Temp;
		Temp = 0;

		return;
	}
	if (MaxAmmo < ClipSize)
	{
		Temp = CurrentAmmo - MaxAmmo;
		if (Temp <= ClipSize)
		{
			CurrentAmmo = CurrentAmmo + MaxAmmo;
			MaxAmmo = 0;
			Temp = 0;
			return;
		}
		if (Temp > ClipSize)
		{
			MaxAmmo = MaxAmmo - CurrentAmmo;
			CurrentAmmo = ClipSize;
		}
	}
}


void ASWeapon::ReloadWeapon()
{
	Reload();
}


void ASWeapon::StartFire()
{
	float FirstDelay = FMath::Max(LastFireTime + TimeBetweenShots - GetWorld()->TimeSeconds, 0.0f);

	GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenShots, this, &ASWeapon::Fire, TimeBetweenShots, true, FirstDelay);
}


void ASWeapon::StopFire()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);
}


void ASWeapon::PlayFireEffects(FVector TraceEnd)
{
	if (MuzzleEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocketName);
	}

	if (TracerEffect)
	{
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		UParticleSystemComponent* TracerComp = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TracerEffect, MuzzleLocation);
		if (TracerComp)
		{
			TracerComp->SetVectorParameter(TracerTargetName, TraceEnd);
		}
	}

	APawn* MyOwner = Cast<APawn>(GetOwner());
	if (MyOwner)
	{
		APlayerController* PC = Cast<APlayerController>(MyOwner->GetController());
		if (PC)
		{
			PC->ClientPlayCameraShake(FireCamShake);
		}
	}
}


void ASWeapon::PlayImpactEffects(EPhysicalSurface SurfaceType, FVector ImpactPoint)
{
	UParticleSystem* SelectedEffect = nullptr;
	switch (SurfaceType)
	{
	case SURFACE_FLESHDEFAULT:
	case SURFACE_FLESHVULNERABLE:
		SelectedEffect = FleshImpactEffect;
		break;
	default:
		SelectedEffect = DefaultImpactEffect;
		break;
	}

	if (SelectedEffect)
	{
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		FVector ShotDirection = ImpactPoint - MuzzleLocation;
		ShotDirection.Normalize();

		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, ImpactPoint, ShotDirection.Rotation());
	}
}


// ------- SERVER FUNCTIONS ------- \\


void ASWeapon::OnRep_HitScanTrace()
{
	// Play cosmetic FX
	PlayFireEffects(HitScanTrace.TraceTo);
	PlayImpactEffects(HitScanTrace.SurfaceType, HitScanTrace.TraceTo);
}


void ASWeapon::ServerFire_Implementation()
{
	Fire();
}


bool ASWeapon::ServerFire_Validate()
{
	return true;
}


void ASWeapon::ServerReload_Implementation()
{
	Reload();
}


bool ASWeapon::ServerReload_Validate()
{
	return true;
}


void ASWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ASWeapon, HitScanTrace, COND_SkipOwner);
}


// ------- EXTRA ------- \\


bool ASWeapon::IsTargeting(bool WasTargeting)
{
	ASCharacter* MyOwner = Cast<ASCharacter>(GetOwner());
	if (!ensure(MyOwner != nullptr)) return false;

	if (WasTargeting)
	{
		//Not Target Any More
		MyOwner->GetCharacterMovement()->MaxWalkSpeed = 600;
		MyOwner->bWantsToZoom = false;

		BulletSpread = DefaultBulletSpread;

		return false;
	}
	else
	{
		//Start To Target
		MyOwner->GetCharacterMovement()->MaxWalkSpeed = 300;
		MyOwner->bWantsToZoom = true;

		BulletSpread = AimBulletSpread;

		return true;
	}

	return false;
}

