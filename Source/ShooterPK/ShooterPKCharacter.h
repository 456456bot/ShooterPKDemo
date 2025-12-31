// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ShooterPKCharacter.generated.h"

class UTP_WeaponComponent;
class USpringArmComponent;
class UInputComponent;
class USkeletalMeshComponent;
class USceneComponent;
class UCameraComponent;
class UAnimMontage;
class USoundBase;
class APlayerState;

// Declaration of the delegate that will be called when the Primary Action is triggered
// It is declared as dynamic so it can be accessed also in Blueprints
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUseItem);

// Declare a Delegate signature to implement the method of reloading
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReload);

UCLASS(config=Game)
class AShooterPKCharacter : public ACharacter {
	GENERATED_BODY()

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	USkeletalMeshComponent* Mesh1P;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* CharacterCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

public:
	AShooterPKCharacter();

protected:
	virtual void BeginPlay() override;

public:
	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float TurnRateGamepad;

	/** Delegate to whom anyone can subscribe to receive this event */
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnUseItem OnUseItem;

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnReload OnReload;

protected:
	/** Fires a projectile. */
	void OnPrimaryAction();

	void StopPrimaryAction();

	UFUNCTION(Server, Reliable)
	void StopPrimaryActionServer();

	UFUNCTION(NetMulticast, Reliable)
	void StopPrimaryActionMulticast();

	UFUNCTION(Server, Reliable)
	void OnPrimaryActionServer();

	UFUNCTION(NetMulticast, Reliable)
	void OnPrimaryActionMulticast();

	// Reload ammo
	void ReloadAmmo();

	UFUNCTION(Server, Reliable)
	void ReloadAmmoServer();

	UFUNCTION(NetMulticast, Reliable)
	void ReloadAmmoMulticast();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void AutoReloadTrigger();

	/** Handles moving forward/backward */
	void MoveForward(float Val);

	/** Handles strafing movement, left and right */
	void MoveRight(float Val);

	/**
	 * Called via input to turn at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	struct TouchData {
		TouchData() {
			bIsPressed = false;
			Location = FVector::ZeroVector;
		}

		bool bIsPressed;
		ETouchIndex::Type FingerIndex;
		FVector Location;
		bool bMoved;
	};

	void BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location);
	void EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location);
	void TouchUpdate(const ETouchIndex::Type FingerIndex, const FVector Location);
	TouchData TouchItem;

	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface

	/* 
	 * Configures input for touchscreen devices if there is a valid touch interface for doing so 
	 *
	 * @param	InputComponent	The input component pointer to bind controls to
	 * @returns true if touch controls were enabled.
	 */
	bool EnableTouchscreenMovement(UInputComponent* InputComponent);

public:
	/** Returns Mesh1P subobject **/
	UFUNCTION(BlueprintCallable, Category="C++ Foo")
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }

	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return CharacterCamera; }

protected:
	/** Movement */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++ Variable")
	float DefaultSpeed = 320.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++ Variable")
	float SprintSpeed = 660.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++ Variable")
	float JumpVelocity = 500.f;

	UFUNCTION(BlueprintCallable)
	void AdjustSpeed();

	void StartSprinting();

	UFUNCTION(Server, Reliable)
	void StartSprintingServer();

	UFUNCTION(NetMulticast, Reliable)
	void StartSprintingMulticast();

	void StopSprinting();

	UFUNCTION(Server, Reliable)
	void StopSprintingServer();

	UFUNCTION(NetMulticast, Reliable)
	void StopSprintingMulticast();

	UPROPERTY(EditAnywhere, Category = "C++ Variable")
	float CrouchSpeed = 160.f;

	void StartCrouching();

	void StopCrouching();

	/** Weapon */
	UPROPERTY(EditAnywhere, Replicated, ReplicatedUsing=OnRep_IsWeaponInHand, BlueprintReadOnly, Category = "C++ Variable")
	bool bIsWeaponInHand;

	UFUNCTION()
	void OnRep_IsWeaponInHand();

	UPROPERTY(EditDefaultsOnly, Category = "C++ Variable")
	UAnimMontage* SwitchWeaponMontage;

	void ToggleWeapon();

	UFUNCTION(Server, Reliable)
	void ToggleWeaponServer();

	UFUNCTION(NetMulticast, Reliable)
	void ToggleWeaponMulticast();

	UFUNCTION(BlueprintCallable)
	void WeaponAttachmentSwitch();

	UPROPERTY(EditDefaultsOnly, Category = "C++ Variable")
	TSubclassOf<AActor> WeaponToSpawn;

	void SetWeaponToSpawn(TSubclassOf<AActor> TargetWeapon);

	void SpawnWeapon1PAnd3P();

	UPROPERTY(Replicated, BlueprintReadOnly)
	UTP_WeaponComponent* CurrentWeapon;

	UPROPERTY(Replicated, BlueprintReadOnly)
	UTP_WeaponComponent* CurrentWeapon1P;

private:
	/** Deprecated */
	void HideHeadIfLocal();

public:
	/** Player state */
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++ Variable")
	// float MaxHealth = 100;
	//
	// UPROPERTY(EditAnywhere, Replicated, ReplicatedUsing=OnRep_Health, BlueprintReadWrite, Category = "C++ Variable")
	// float Health = 100;
	//
	// UFUNCTION(BlueprintCallable)
	// void GetDamage(float Damage);
	//
	// UFUNCTION()
	// void OnRep_Health();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void BP_UpdateHealthUI();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void BP_UpdateAmmoUI();

	UFUNCTION(BlueprintCallable)
	void SetDeath();

	UPROPERTY(EditAnywhere, Category = "C++ Variable")
	int PlayerID;

	UPROPERTY(EditAnywhere, Category = "C++ Variable")
	bool PlantingBomb;

protected:
	/** Network related */
	virtual void PossessedBy(AController* NewController) override;

	virtual void OnRep_PlayerState() override;

private:
	void InitPlayerInfo();

public:
	// The sound played when player dies
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="C++ Variable")
	USoundBase* DeathSound;
};
