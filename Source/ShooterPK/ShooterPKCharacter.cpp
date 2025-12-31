// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterPKCharacter.h"
#include "ShooterPK.h"
#include "ShooterPKProjectile.h"
#include "TP_WeaponComponent.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/ChildActorComponent.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"

//////////////////////////////////////////////////////////////////////////
// AShooterPKCharacter

AShooterPKCharacter::AShooterPKCharacter() {
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	TurnRateGamepad = 45.f;

	// Create a SpringArmComponent
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f));
	CameraBoom->TargetArmLength = 0.0f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bDoCollisionTest = false;

	// Create a CameraComponent	
	CharacterCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CharacterCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	// CharacterCamera->SetupAttachment(RootComponent);
	CharacterCamera->SetRelativeLocation(FVector(0, 0, 0)); // Position the camera
	CharacterCamera->bUsePawnControlRotation = false;
	CharacterCamera->SetFieldOfView(100);

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(CharacterCamera);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));

	// Set Player state
	// Health = MaxHealth;
}

void AShooterPKCharacter::BeginPlay() {
	// Call the base class  
	Super::BeginPlay();

	// float Scale = 2;
	// SetActorScale3D(FVector(Scale, Scale, Scale));

	// Hide the character head for local player
	// HideHeadIfLocal();

	/** Spawn weapon for player */
	SpawnWeapon1PAnd3P();

	// Adjust Speed
	AdjustSpeed();

	// OnUseItem.AddDynamic(this, &AShooterPKCharacter::BP_UpdateHealthUI);
	//
	// BP_UpdateHealthUI();
}

void AShooterPKCharacter::AdjustSpeed() {
	GetCharacterMovement()->MaxWalkSpeed = DefaultSpeed;
	GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
	GetCharacterMovement()->JumpZVelocity = JumpVelocity;
}


void AShooterPKCharacter::PossessedBy(AController* NewController) {
	Super::PossessedBy(NewController);

	InitPlayerInfo();
}

void AShooterPKCharacter::OnRep_PlayerState() {
	Super::OnRep_PlayerState();

	InitPlayerInfo();
}

void AShooterPKCharacter::InitPlayerInfo() {
	APlayerState* PS = GetPlayerState();
	if (PS) {
		PlayerID = PS->GetPlayerId();
		// PRINT("PlayerState Init Success: %d， %s", PlayerID, *UEnum::GetValueAsString(TEXT("Engine.ENetRole"),GetLocalRole()));
	}
}

void AShooterPKCharacter::SetWeaponToSpawn(TSubclassOf<AActor> TargetWeapon) {
	WeaponToSpawn = TargetWeapon;
}

void AShooterPKCharacter::SpawnWeapon1PAnd3P() {
	if (HasAuthority()) {
		if (WeaponToSpawn != nullptr) {
			UWorld* const World = GetWorld();
			if (World != nullptr) {
				AActor* WeaponActor = World->SpawnActor<AActor>(WeaponToSpawn);
				if (WeaponActor) {
					CurrentWeapon1P = WeaponActor->FindComponentByClass<UTP_WeaponComponent>();
					if (CurrentWeapon1P) {
						CurrentWeapon1P->Initialize(this, true);
					}
				}

				WeaponActor = World->SpawnActor<AActor>(WeaponToSpawn);
				if (WeaponActor) {
					CurrentWeapon = WeaponActor->FindComponentByClass<UTP_WeaponComponent>();
					if (CurrentWeapon) {
						CurrentWeapon->Initialize(this);
					}
				}
			}
		}
	}
}

void AShooterPKCharacter::HideHeadIfLocal() {
	if (GetMesh() && IsLocallyControlled()) {
		GetMesh()->HideBoneByName(FName("head"), PBO_None);
		GetMesh()->bCastHiddenShadow = true;
	}
}

//////////////////////////////////////////////////////////////////////////// Input

void AShooterPKCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) {
	// Set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind fire event
	PlayerInputComponent->BindAction("PrimaryAction", IE_Pressed, this, &AShooterPKCharacter::OnPrimaryAction);
	PlayerInputComponent->BindAction("PrimaryAction", IE_Released, this, &AShooterPKCharacter::StopPrimaryAction);

	// Enable touchscreen input
	EnableTouchscreenMovement(PlayerInputComponent);

	// Bind movement events
	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &AShooterPKCharacter::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &AShooterPKCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "Mouse" versions handle devices that provide an absolute delta, such as a mouse.
	// "Gamepad" versions are for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Turn Right / Left Gamepad", this, &AShooterPKCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("Look Up / Down Gamepad", this, &AShooterPKCharacter::LookUpAtRate);

	// Bind sprinting action (Shift key press/release)
	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AShooterPKCharacter::StartSprinting);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AShooterPKCharacter::StopSprinting);

	// Bind crouching action (Ctrl key press/release)
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AShooterPKCharacter::StartCrouching);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &AShooterPKCharacter::StopCrouching);

	// Bind toggling weapon action (Q key press)
	PlayerInputComponent->BindAction("ToggleWeapon", IE_Pressed, this, &AShooterPKCharacter::ToggleWeapon);

	// Bind reloading ammo action (R key press)
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &AShooterPKCharacter::ReloadAmmo);
}

void AShooterPKCharacter::StartSprinting() {
	StartSprintingServer();
}

void AShooterPKCharacter::StartSprintingServer_Implementation() {
	StartSprintingMulticast();
}

void AShooterPKCharacter::StartSprintingMulticast_Implementation() {
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void AShooterPKCharacter::StopSprinting() {
	StopSprintingServer();
}

void AShooterPKCharacter::StopSprintingServer_Implementation() {
	StopSprintingMulticast();
}

void AShooterPKCharacter::StopSprintingMulticast_Implementation() {
	GetCharacterMovement()->MaxWalkSpeed = DefaultSpeed;
}

void AShooterPKCharacter::StartCrouching() {
	GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
	Crouch();
	CameraBoom->SetRelativeLocation(FVector(15, 1.75f, 60));
}

void AShooterPKCharacter::StopCrouching() {
	UnCrouch();
	CameraBoom->SetRelativeLocation(FVector(25, 1.75f, 60));
}

void AShooterPKCharacter::ToggleWeapon() {
	if (PlantingBomb) {
		return;
	}
	if (CurrentWeapon && CurrentWeapon1P) {
		if (bIsWeaponInHand && CurrentWeapon1P->bIsReady || !bIsWeaponInHand) {
			ToggleWeaponServer();
		}
	}
}

void AShooterPKCharacter::ToggleWeaponServer_Implementation() {
	ToggleWeaponMulticast();
}

void AShooterPKCharacter::ToggleWeaponMulticast_Implementation() {
	// Play animation
	if (SwitchWeaponMontage) {
		PlayAnimMontage(SwitchWeaponMontage, 3);
	}
}

void AShooterPKCharacter::WeaponAttachmentSwitch() {
	if (HasAuthority()) {
		bIsWeaponInHand = !bIsWeaponInHand;

		OnRep_IsWeaponInHand();
	}
}

void AShooterPKCharacter::OnRep_IsWeaponInHand() {
	if (CurrentWeapon && CurrentWeapon1P) {
		if (!bIsWeaponInHand) {
			CurrentWeapon->AttachToBack();
			CurrentWeapon1P->AttachToBack();

			GetCharacterMovement()->MaxWalkSpeed *= 1.5f;
			SprintSpeed *= 1.5f;
			DefaultSpeed *= 1.5f;
		} else {
			CurrentWeapon->AttachToHand();
			CurrentWeapon1P->AttachToHand();

			GetCharacterMovement()->MaxWalkSpeed /= 1.5f;
			SprintSpeed /= 1.5f;
			DefaultSpeed /= 1.5f;
		}
	}
}


void AShooterPKCharacter::OnPrimaryAction() {
	if (PlantingBomb) {
		return;
	}
	OnPrimaryActionServer();
}

void AShooterPKCharacter::OnPrimaryActionServer_Implementation() {
	OnPrimaryActionMulticast();
}

void AShooterPKCharacter::OnPrimaryActionMulticast_Implementation() {
	// Trigger the OnItemUsed Event
	OnUseItem.Broadcast();
}

void AShooterPKCharacter::StopPrimaryAction() {
	StopPrimaryActionServer();
}

void AShooterPKCharacter::StopPrimaryActionServer_Implementation() {
	StopPrimaryActionMulticast();
}

void AShooterPKCharacter::StopPrimaryActionMulticast_Implementation() {
	if (CurrentWeapon && CurrentWeapon1P) {
		CurrentWeapon1P->StopFiring();
		CurrentWeapon->StopFiring();
	}
}

void AShooterPKCharacter::ReloadAmmo() {
	if (PlantingBomb) {
		return;
	}
	ReloadAmmoServer();
}

void AShooterPKCharacter::ReloadAmmoServer_Implementation() {
	ReloadAmmoMulticast();
}

void AShooterPKCharacter::ReloadAmmoMulticast_Implementation() {
	OnReload.Broadcast();
}

void AShooterPKCharacter::BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location) {
	if (TouchItem.bIsPressed == true) {
		return;
	}
	if ((FingerIndex == TouchItem.FingerIndex) && (TouchItem.bMoved == false)) {
		OnPrimaryAction();
	}
	TouchItem.bIsPressed = true;
	TouchItem.FingerIndex = FingerIndex;
	TouchItem.Location = Location;
	TouchItem.bMoved = false;
}

void AShooterPKCharacter::EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location) {
	if (TouchItem.bIsPressed == false) {
		return;
	}
	TouchItem.bIsPressed = false;
}

void AShooterPKCharacter::MoveForward(float Value) {
	if (Value != 0.0f) {
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AShooterPKCharacter::MoveRight(float Value) {
	if (Value != 0.0f) {
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AShooterPKCharacter::TurnAtRate(float Rate) {
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void AShooterPKCharacter::LookUpAtRate(float Rate) {
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

bool AShooterPKCharacter::EnableTouchscreenMovement(class UInputComponent* PlayerInputComponent) {
	if (FPlatformMisc::SupportsTouchInput() || GetDefault<UInputSettings>()->bUseMouseForTouch) {
		PlayerInputComponent->BindTouch(IE_Pressed, this, &AShooterPKCharacter::BeginTouch);
		PlayerInputComponent->BindTouch(IE_Released, this, &AShooterPKCharacter::EndTouch);

		return true;
	}

	return false;
}

// void AShooterPKCharacter::GetDamage(float Damage) {
// 	if (Health <= 0) {
// 		return;
// 	}
// 	if (Health <= Damage) {
// 		Health = 0;
// 		SetDeath();
// 		return;
// 	}
//
// 	Health -= Damage;
// 	BP_UpdateHealthUI();
// }
//
// void AShooterPKCharacter::OnRep_Health() {
// 	if (Health <= 0) {
// 		SetDeath();
// 	}
// 	BP_UpdateHealthUI();
// }


void AShooterPKCharacter::SetDeath() {
	// Stop animation
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance()) {
		AnimInstance->StopAllMontages(0.0f);
	}
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	if (UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance()) {
		AnimInstance->StopAllMontages(0.0f);
	}
	Mesh1P->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	// Play sound
	if (DeathSound) {
		UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
	}
	// PRINT("Death")

	// Disable the controller and let the character stop immediately
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->SetComponentTickEnabled(false);
	if (APlayerController* PC = Cast<APlayerController>(GetController())) {
		DisableInput(PC);
	}

	// Disable Replication
	SetReplicateMovement(false);

	// Activate 3P mesh
	GetMesh()->SetOwnerNoSee(false);

	// Deactivate 1P mesh
	GetMesh1P()->SetVisibility(false, true);

	// Move the camera behind the character
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepWorldTransform, FName("Pelvis"));
	CameraBoom->SetUsingAbsoluteRotation(true);
	FRotator DeathCameraRotation = FRotator(-60.0f, GetActorRotation().Yaw, 0.0f);
	CameraBoom->SetWorldRotation(DeathCameraRotation);
	CameraBoom->TargetArmLength = 300.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 0.0f);
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 2.0f;

	// Activate the ragdoll physics
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetLinearDamping(0.5f);
	GetMesh()->SetAngularDamping(0.5f);

	// Clear weapons
	CurrentWeapon1P->GetOwner()->Destroy();
	CurrentWeapon->SetDeathConfig();

	// Destroy the character (Delayed)
	SetLifeSpan(5.0f);
}

void AShooterPKCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterPKCharacter, CurrentWeapon1P);
	DOREPLIFETIME(AShooterPKCharacter, CurrentWeapon);
	DOREPLIFETIME(AShooterPKCharacter, bIsWeaponInHand);
}
