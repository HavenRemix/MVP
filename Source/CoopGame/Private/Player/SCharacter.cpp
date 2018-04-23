// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/SCharacter.h"
#include "Player/SPlayerController.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "CoopGame.h"
#include "SHealthComponent.h"
#include "Weapon/SWeapon.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Math/Vector.h"
#include "GameFramework/Pawn.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"
#include "Containers/Array.h"
#include "Math/UnrealMathUtility.h"
#include "TimerManager.h"
#include "GameFramework/SpringArmComponent.h"




ASCharacter::ASCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

//Default Mesh

	GetMesh()->bCastDynamicShadow = true;
	GetMesh()->CastShadow = true;

//Spring Arm

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->bUsePawnControlRotation = true;
	SpringArmComp->SetupAttachment(RootComponent);

//Camera Comp

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp);

//Defaults

	GetMovementComponent()->GetNavAgentPropertiesRef().bCanCrouch = true;
	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));

	ZoomedFOV = 35.0f;
	ZoomInterpSpeed = 18;

	WeaponAttachSocketName = "WeaponSocket";
	WeaponBackAttachSocketName = "WeaponBackSocket";

	IsRifleType = true;
	bReloading = false;
}


void ASCharacter::BeginPlay()
{
	Super::BeginPlay();

	DefaultFOV = CameraComp->FieldOfView;
	HealthComp->OnHealthChanged.AddDynamic(this, &ASCharacter::OnHealthChanged);

	//Play Song
	if (IsLocallyControlled() && MusicTrack)
	{
		UGameplayStatics::PlaySoundAtLocation(this, MusicTrack, GetActorLocation());
	}

	EquipWeapon(Weapons[0]);
	if (CurrentWeapon)
	{
		IsRifleType = CurrentWeapon->IsRifle;
	}
}


void ASCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	float TargetFOV = bWantsToZoom ? ZoomedFOV : DefaultFOV;
	float NewFOV = FMath::FInterpTo(CameraComp->FieldOfView, TargetFOV, DeltaTime, ZoomInterpSpeed);

	CameraComp->SetFieldOfView(NewFOV);
}


void ASCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ASCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASCharacter::MoveRight);

	PlayerInputComponent->BindAxis("LookUp", this, &ASCharacter::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Turn", this, &ASCharacter::AddControllerYawInput);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ASCharacter::BeginCrouch);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &ASCharacter::EndCrouch);

	PlayerInputComponent->BindAction("Zoom", IE_Pressed, this, &ASCharacter::Zoom);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ASCharacter::StartFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ASCharacter::StopFire);

	PlayerInputComponent->BindAction("Run", IE_Pressed, this, &ASCharacter::BeginRun);
	PlayerInputComponent->BindAction("Run", IE_Released, this, &ASCharacter::EndRun);

	PlayerInputComponent->BindAction("NextWeapon", IE_Pressed, this, &ASCharacter::NextWeaponInput);
	PlayerInputComponent->BindAction("PreviousWeapon", IE_Released, this, &ASCharacter::PreviousWeaponInput);

	PlayerInputComponent->BindAction("Reload", IE_Released, this, &ASCharacter::Reload);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
}


// ------- INPUT ------- \\


void ASCharacter::MoveForward(float Value)
{
	AddMovementInput(GetActorForwardVector() * Value);
}


void ASCharacter::MoveRight(float Value)
{
	AddMovementInput(GetActorRightVector() * Value);
}


void ASCharacter::BeginCrouch()
{
	Crouch();
}


void ASCharacter::EndCrouch()
{
	UnCrouch();
}


void ASCharacter::Zoom()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->IsTargeting(bWantsToZoom);
	}
}


void ASCharacter::BeginRun()
{
	if (bWantsToZoom)
	{
		CurrentWeapon->IsTargeting(bWantsToZoom);
		GetCharacterMovement()->MaxWalkSpeed = 1000;
	}
	else {
		GetCharacterMovement()->MaxWalkSpeed = 1000;
	}

	bIsRunning = true;
}


void ASCharacter::EndRun()
{
	GetCharacterMovement()->MaxWalkSpeed = 600;

	bIsRunning = false;
}


void ASCharacter::StartFire()
{
	if (bIsRunning)
	{
		EndRun();
	}
	if (CurrentWeapon)
	{
		CurrentWeapon->StartFire();
	}
}


void ASCharacter::StopFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFire();
	}
}


void ASCharacter::Reload()
{
	if (bWantsToZoom)
	{
		CurrentWeapon->IsTargeting(bWantsToZoom);
	}
	if (bIsRunning)
	{
		EndRun();
	}
	bReloading = true;
	GetWorldTimerManager().SetTimer(TimerHandle_ReloadTimer, this, &ASCharacter::FinishAction, 1.4f, false, 0.0f);
}


// ------- WEAPON LOGIC ------- \\


void ASCharacter::EquipWeapon(TSubclassOf<ASWeapon> Weapon)
{
	if (Role == ROLE_Authority)
	{
		ASWeapon* PlaceHolderWeapon = CurrentWeapon;

		//Spawn Rules
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (UnequippedWeapon)
		{
			//Switch Weapon
			CurrentWeapon = UnequippedWeapon;
			UnequippedWeapon = PlaceHolderWeapon;
		}
		else
		{
			//Spawn New Current Weapon
			CurrentWeapon = GetWorld()->SpawnActor<ASWeapon>(Weapon, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
			UnequippedWeapon = PlaceHolderWeapon;
		}

		//Attach the weapon to the socket
		if (CurrentWeapon)
		{
			CurrentWeapon->SetOwner(this);
			CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponAttachSocketName);
			CurrentWeapon->SetActorLocation(GetMesh()->GetSocketLocation(WeaponAttachSocketName));

			IsRifleType = CurrentWeapon->IsRifle;
		}
		if (UnequippedWeapon)
		{
			UnequippedWeapon->SetOwner(this);
			UnequippedWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponBackAttachSocketName);
			UnequippedWeapon->SetActorLocation(GetMesh()->GetSocketLocation(WeaponBackAttachSocketName));
		}
	}

}


void ASCharacter::NextWeaponInput()
{
	EquipWeapon(Weapons[1]);
}


void ASCharacter::PreviousWeaponInput()
{
	EquipWeapon(Weapons[0]);
}


// ------- FUNCTIONS ------- \\


void ASCharacter::OnHealthChanged(USHealthComponent* OwningHealthComp, float Health, float HealthDelta, const class UDamageType* DamageType,
	class AController* InstigatedBy, AActor* DamageCauser)
{
	if (Health <= 0.0f && !bDied)
	{
		// Die!
		bDied = true;

		if (CurrentWeapon != nullptr)
		{
			CurrentWeapon->Destroy();
		}

		GetMovementComponent()->StopMovementImmediately();
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		DetachFromControllerPendingDestroy();
		SetLifeSpan(10.0f);

		class ASPlayerController* PC = Cast<ASPlayerController>(GetController());
		PC->RespawnPlayer(5.0f, false);
	}
}


FVector ASCharacter::GetPawnViewLocation() const
{
	if (CameraComp)
	{
		return CameraComp->GetComponentLocation();
	}

	return Super::GetPawnViewLocation();
}


FRotator ASCharacter::GetAimOffsets() const
{
	const FVector AimDirWS = GetBaseAimRotation().Vector();
	const FVector AimDirLS = ActorToWorld().InverseTransformVectorNoScale(AimDirWS);
	const FRotator AimRotLS = AimDirLS.Rotation();

	return AimRotLS;
}


void ASCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCharacter, CurrentWeapon);
	DOREPLIFETIME(ASCharacter, bDied);
}


void ASCharacter::FinishAction()
{
	if (TimerHandle_ReloadTimer.IsValid() && CurrentWeapon)
	{
		CurrentWeapon->ReloadWeapon();
		bReloading = false;
	}
}
