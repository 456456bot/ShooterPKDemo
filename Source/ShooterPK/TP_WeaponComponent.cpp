// Copyright Epic Games, Inc. All Rights Reserved.


#include "TP_WeaponComponent.h"
#include "ShooterPK.h"
#include "ShooterPKCharacter.h"
#include "ShooterPKProjectile.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UTP_WeaponComponent::UTP_WeaponComponent() {
	// Default offset from the character location for projectiles to spawn
	MuzzleOffset = FVector(100.0f, 0.0f, 10.0f);
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UTP_WeaponComponent::BeginPlay() {
	// Call the base class  
	Super::BeginPlay();

	InitialReservedAmmo = ReservedAmmo;
}

void UTP_WeaponComponent::SetCharacter(AShooterPKCharacter* NewCharacter) {
	Character = NewCharacter;
	// Register so that Fire is called every time the character tries to use the item being held
}

void UTP_WeaponComponent::SetReady(bool NewState) {
	bIsReady = NewState && CurrentAmmo > 0;
}

void UTP_WeaponComponent::Initialize(AShooterPKCharacter* TargetCharacter, bool bIs1P) {
	SetCharacter(TargetCharacter);
	bIs1PWeapon = bIs1P;

	OnRep_CharacterThenInitializeOnAllClients();

	if (Character && bIs1PWeapon) {
		Character->BP_UpdateAmmoUI();
	}

	TArray<UStaticMeshComponent*> AllMeshes;
	GetOwner()->GetComponents<UStaticMeshComponent>(AllMeshes);
	for (auto Mesh : AllMeshes) {
		Mesh->SetCollisionProfileName(FName("NoCollision"));
	}
}

void UTP_WeaponComponent::OnRep_CharacterThenInitializeOnAllClients() {
	GetOwner()->SetOwner(Character);

	TArray<UStaticMeshComponent*> AllMeshes;
	GetOwner()->GetComponents<UStaticMeshComponent>(AllMeshes);

	if (bIs1PWeapon) {
		// 1P
		for (auto Mesh : AllMeshes) {
			Mesh->SetOnlyOwnerSee(true);
			Mesh->CastShadow = false;
		}
	} else {
		// 3P
		for (auto Mesh : AllMeshes) {
			Mesh->SetOwnerNoSee(true);
			Mesh->CastShadow = true;
		}
	}

	if (Character) {
		CharacterMesh = bIs1PWeapon ? Character->GetMesh1P() : Character->GetMesh();
		Character->OnReload.AddDynamic(this, &UTP_WeaponComponent::Reload);
		Character->OnUseItem.AddDynamic(this, &UTP_WeaponComponent::Fire);
	}

	AttachToBack();
}

void UTP_WeaponComponent::AttachToBack() {
	FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
	GetOwner()->AttachToComponent(CharacterMesh, AttachmentRules, BackSocketName);

	// The weapon1P should be invisible if it's on the back
	if (bIs1PWeapon) {
		TArray<UStaticMeshComponent*> AllMeshes;
		SetVisible(false);
	}

	SetReady(false);
}

void UTP_WeaponComponent::AttachToHand() {
	FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
	GetOwner()->AttachToComponent(CharacterMesh, AttachmentRules, HandSocketName);
	if (bIs1PWeapon) {
		// The weapon1P should be visible if it's on the hand
		SetVisible(true);
	}
	SetReady(true);
}

void UTP_WeaponComponent::SetDeathConfig() {
	TArray<UStaticMeshComponent*> AllMeshes;
	GetOwner()->GetComponents<UStaticMeshComponent>(AllMeshes);
	for (auto Mesh : AllMeshes) {
		Mesh->SetOwnerNoSee(false);
		Mesh->SetOnlyOwnerSee(false);
	}

	GetOwner()->SetLifeSpan(4.9f);
}

void UTP_WeaponComponent::Fire() {
	if (bIs1PWeapon) {
		if (!bIsReady) {
			return;
		}

		if (Character == nullptr) {
			return;
		}

		bIsFiring = true;

		// Try and play the sound if specified
		if (FireSound != nullptr) {
			UGameplayStatics::PlaySoundAtLocation(this, FireSound, Character->GetActorLocation());
		}

		// Try and play a firing animation if specified
		if (FireAnimation != nullptr) {
			// Get the animation object for the arms mesh
			UAnimInstance* AnimInstance = CharacterMesh->GetAnimInstance();
			if (AnimInstance != nullptr) {
				AnimInstance->Montage_Play(FireAnimation, FireAnimRate);
			}
		}

		// Try and fire a projectile (local client do)
		if (Character->IsLocallyControlled()) {
			OnWeaponSpawnProjectile.Broadcast();
			// Recoil
			if (TargetRecoilPitch < RecoilPitchBoundary) {
				TargetRecoilPitch += RecoilStrengthPitch;
			}

			float RandomYaw = RecoilStrengthYaw;
			float Direction = (FMath::RandBool()) ? 1.0f : -1.0f;
			float PredictedYaw = TargetRecoilYaw + (RandomYaw * Direction);
			if (FMath::Abs(PredictedYaw) > RecoilYawBoundary) {
				Direction *= -1.0f;
			}
			TargetRecoilYaw += (RandomYaw * Direction);
			// if (APlayerController* PC = Cast<APlayerController>(Character->GetController())) {
			// 	// Pitch: Up only
			// 	if (CurrentRecoilPitch < RecoilPitchBoundary) {
			// 		PC->AddPitchInput(-RecoilStrengthPitch);
			// 		CurrentRecoilPitch += RecoilStrengthPitch;
			// 	}
			// 	
			// 	// Yaw: Left and right within a boundary
			// 	int Direction = FMath::RandBool() ? 1 : -1;
			// 	if (FMath::Abs(CurrentRecoilYaw + RecoilStrengthYaw * Direction) > RecoilYawBoundary) {
			// 		Direction = -1 * Direction;
			// 	}
			// 	PC->AddYawInput(RecoilStrengthYaw * Direction);
			// 	CurrentRecoilYaw += RecoilStrengthYaw * Direction;
			// }
		}

		// Set Ammo
		CurrentAmmo -= 1;

		if (AutoFiringRate > 0) {
			GetWorld()->GetTimerManager().SetTimer(
				AutoFiringTimer,
				this,
				&UTP_WeaponComponent::Fire,
				AutoFiringRate,
				true
			);
		} else {
			// Stop the player from firing continuously
			bIsReady = false;
		}

		// Update UI
		Character->BP_UpdateAmmoUI();
	} else {
		if (!bIsReady) {
			return;
		}

		if (Character == nullptr) {
			return;
		}

		// Try and play a firing animation if specified
		if (FireAnimation != nullptr) {
			// Get the animation object for the arms mesh
			UAnimInstance* AnimInstance = CharacterMesh->GetAnimInstance();
			if (AnimInstance != nullptr) {
				AnimInstance->Montage_Play(FireAnimation, FireAnimRate);
			}
		}

		// Set Ammo
		CurrentAmmo -= 1;

		if (AutoFiringRate > 0) {
			GetWorld()->GetTimerManager().SetTimer(
				AutoFiringTimer,
				this,
				&UTP_WeaponComponent::Fire,
				AutoFiringRate,
				true
			);
		} else {
			// Stop the player from firing continuously
			bIsReady = false;
		}
	}
}

void UTP_WeaponComponent::StopFiring() {
	bIsFiring = false;
	GetWorld()->GetTimerManager().ClearTimer(AutoFiringTimer);
}

void UTP_WeaponComponent::Reload() {
	if (CurrentAmmo >= AmmoSize) return;
	SetReady(false);
	if (ReservedAmmo > 0) {
		// Try and play a reloading animation if specified
		if (ReloadAnimation != nullptr) {
			// Get the animation object for the arms mesh
			UAnimInstance* AnimInstance = CharacterMesh->GetAnimInstance();
			if (AnimInstance != nullptr) {
				// PRINT("reload-weapon1P: %d--PlayerID: %d", bIs1PWeapon, Character->PlayerID)
				if (bIs1PWeapon) {
					AnimInstance->Montage_Play(ReloadAnimation1P, ReloadAnimRate);
				} else {
					AnimInstance->Montage_Play(ReloadAnimation, ReloadAnimRate);
				}
			}
		}
	}
}

void UTP_WeaponComponent::ReloadDone() {
	int Needed = AmmoSize - CurrentAmmo;

	int AmountToLoad = FMath::Min(Needed, ReservedAmmo);

	ReservedAmmo -= AmountToLoad;
	CurrentAmmo += AmountToLoad;

	SetReady(true);

	// Update UI
	Character->BP_UpdateAmmoUI();
}


// void UTP_WeaponComponent::SpawnProjectile() {
// 	// if (GetOwner()->HasAuthority()) {
// 	// 	PRINT("Local Authority: Server");
// 	// } else {
// 	// 	PRINT("Local Authority: Client");
// 	// }
//
// 	// 检查武器组件的 NetRole
// 	// PRINT("Component Net Role: %s", *UEnum::GetValueAsString(TEXT("Engine.ENetRole"), GetOwnerRole()));
// 	SpawnProjectileServer();
// 	// PRINT("SpawnProjectile");
// }

// void UTP_WeaponComponent::SpawnProjectileServer_Implementation() {
// 	// PRINT("Father")
// 	SpawnProjectileMulticast(FVector(), FRotator());
// }
//
// void UTP_WeaponComponent::SpawnProjectileMulticast_Implementation(const FVector& SpawnLocation, const FRotator& SpawnRotation) {
// 	PRINT("Not Implemented. ")
// 	// if (ProjectileClass != nullptr) {
// 	// 	UWorld* const World = GetWorld();
// 	// 	if (World != nullptr) {
// 	// 		APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
// 	// 		const FRotator SpawnRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
// 	// 		// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
// 	// 		const FVector SpawnLocation = GetOwner()->GetActorLocation() + SpawnRotation.RotateVector(MuzzleOffset);
// 	//
// 	// 		//Set Spawn Collision Handling Override
// 	// 		FActorSpawnParameters ActorSpawnParams;
// 	// 		ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
// 	//
// 	// 		// Spawn the projectile at the muzzle
// 	// 		World->SpawnActor<AActor>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);
// 	// 	}
// 	// }
// }

void UTP_WeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	if (Character != nullptr) {
		// Unregister from the OnUseItem Event
		Character->OnUseItem.RemoveDynamic(this, &UTP_WeaponComponent::Fire);
	}
}

void UTP_WeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UTP_WeaponComponent, bIsReady);
	DOREPLIFETIME(UTP_WeaponComponent, Character);
	DOREPLIFETIME(UTP_WeaponComponent, bIs1PWeapon);
}

// void UTP_WeaponComponent::AttachWeapon(AShooterPKCharacter* TargetCharacter) {
// 	Character = TargetCharacter;
// 	if (Character != nullptr) {
// 		// Attach the weapon to the First Person Character
// 		FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
// 		GetOwner()->AttachToComponent(Character->GetMesh(), AttachmentRules, FName(TEXT("weapon_attach_hand_r")));
// 		GetOwner()->SetOwner(Character);
// 		WeaponMesh->SetOnlyOwnerSee(true);
// 		
// 		// Register so that Fire is called every time the character tries to use the item being held
// 		Character->OnUseItem.AddDynamic(this, &UTP_WeaponComponent::Fire);
// 	}
// }

void UTP_WeaponComponent::SetVisible(const bool NewVisible) {
	TArray<UStaticMeshComponent*> AllMeshes;
	GetOwner()->GetComponents<UStaticMeshComponent>(AllMeshes);
	for (auto Mesh : AllMeshes) {
		Mesh->SetVisibility(NewVisible);
	}
}

/** Recoil logic */
void UTP_WeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIs1PWeapon && Character && Character->IsLocallyControlled()) {
		APlayerController* PC = Cast<APlayerController>(Character->GetController());
		if (!PC) {
			return;
		}

		// If not firing, recover the aim
		if (!bIsFiring) {
			TargetRecoilPitch = FMath::FInterpTo(TargetRecoilPitch, 0.0f, DeltaTime, RecoilRecoverySpeed);
			TargetRecoilYaw = FMath::FInterpTo(TargetRecoilYaw, 0.0f, DeltaTime, RecoilRecoverySpeed);
		}

		// Set recoil
		float NewRecoilPitch = FMath::FInterpTo(CurrentRecoilPitch, TargetRecoilPitch, DeltaTime, RecoilSnappiness);
		float NewRecoilYaw = FMath::FInterpTo(CurrentRecoilYaw, TargetRecoilYaw, DeltaTime, RecoilSnappiness);
		float DeltaPitch = NewRecoilPitch - CurrentRecoilPitch;
		float DeltaYaw = NewRecoilYaw - CurrentRecoilYaw;

		PC->AddPitchInput(-DeltaPitch);
		PC->AddYawInput(DeltaYaw);

		CurrentRecoilPitch = NewRecoilPitch;
		CurrentRecoilYaw = NewRecoilYaw;
	}
}

void UTP_WeaponComponent::RefillAmmo() {
	CurrentAmmo = AmmoSize;
	ReservedAmmo = InitialReservedAmmo;

	Character->BP_UpdateAmmoUI();
	SetReady(true);
}

void UTP_WeaponComponent::GotExtraAmmo() {
	InitialReservedAmmo += AmmoSize * 2;

	RefillAmmo();
}

void UTP_WeaponComponent::Enhance() {
	AmmoSize += 1;
	OnWeaponEnhanced.Broadcast();
}
