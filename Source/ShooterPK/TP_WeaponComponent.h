// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TP_WeaponComponent.generated.h"

class AShooterPKCharacter;

// Declare a Delegate signature to implement the method of generating projectiles in actor blueprints
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponSpawnProjectile);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponEnhanced);

UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SHOOTERPK_API UTP_WeaponComponent : public UActorComponent {
	GENERATED_BODY()

public:
	/** Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=Projectile)
	TSubclassOf<AActor> ProjectileClass;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	USoundBase* FireSound;

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* FireAnimation;

	/** AnimMontage to play each time we reload ammo */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* ReloadAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* ReloadAnimation1P;

	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	FVector MuzzleOffset;

	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Custom)
	// USkeletalMeshComponent* WeaponMesh;

	/** Sets default values for this component's properties */
	UTP_WeaponComponent();

	/** Attaches the actor to a FirstPersonCharacter */
	// UFUNCTION(BlueprintCallable, Category="Weapon")
	// void AttachWeapon(AShooterPKCharacter* TargetCharacter);

	/** Make the weapon Fire a Projectile */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void Fire();

	UFUNCTION(BlueprintCallable)
	void StopFiring();

	UPROPERTY(BlueprintAssignable)
	FOnWeaponSpawnProjectile OnWeaponSpawnProjectile;

	UPROPERTY(BlueprintAssignable)
	FOnWeaponEnhanced OnWeaponEnhanced;

	// UFUNCTION(BlueprintImplementableEvent)
	// void BP_SpawnProjectile();

	// UFUNCTION(Server, Reliable)
	// virtual void SpawnProjectileServer();
	//
	// UFUNCTION(NetMulticast, Reliable)
	// virtual void SpawnProjectileMulticast(const FVector& SpawnLocation, const FRotator& SpawnRotation);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="C++ Variable")
	float FireAnimRate = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="C++ Variable")
	float ReloadAnimRate = 1;

protected:
	virtual void BeginPlay() override;
	/** Ends gameplay for this component. */
	UFUNCTION()
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	/** The Character holding this weapon*/
	UPROPERTY(Replicated, BlueprintReadOnly, ReplicatedUsing=OnRep_CharacterThenInitializeOnAllClients)
	AShooterPKCharacter* Character;

	UPROPERTY(EditAnywhere, Replicated, Category="Replication")
	bool bIsReady = false;

	// 1P weapons execute functions, while 3P weapons only synchronize animations
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "C++ Variable")
	bool bIs1PWeapon;

	void SetCharacter(AShooterPKCharacter* NewCharacter);

	UFUNCTION(BlueprintCallable)
	void SetReady(bool NewState);

	void Initialize(AShooterPKCharacter* TargetCharacter, bool bIs1PWeapon = false);

	UFUNCTION()
	void OnRep_CharacterThenInitializeOnAllClients();

	USkeletalMeshComponent* CharacterMesh;

	void AttachToBack();

	void AttachToHand();

	UPROPERTY(EditAnywhere, Category = "C++ Variable")
	FName BackSocketName = FName("attach_back");

	UPROPERTY(EditAnywhere, Category = "C++ Variable")
	FName HandSocketName = FName("attach_hand_r");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "C++ Variable")
	FName SpecialSocketName = FName("attach_hand_l");

	void SetDeathConfig();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "C++ Variable")
	int CurrentAmmo;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "C++ Variable")
	int AmmoSize;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "C++ Variable")
	int ReservedAmmo;

	UFUNCTION(BlueprintCallable)
	void Reload();

	UFUNCTION(BlueprintCallable)
	void ReloadDone();

	UFUNCTION(BlueprintCallable)
	void SetVisible(const bool NewVisible);

	// Refiring logic for rifle
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++ Variable")
	float AutoFiringRate = -1;

	FTimerHandle AutoFiringTimer;

	/** Recoil logic */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
	float RecoilStrengthPitch = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
	float RecoilStrengthYaw = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
	float RecoilPitchBoundary = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
	float RecoilYawBoundary = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
	float RecoilRecoverySpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
	float RecoilSnappiness = 20.0f;

protected:
	float TargetRecoilPitch = 0.0f;
	float TargetRecoilYaw = 0.0f;

	float CurrentRecoilPitch = 0.0f;
	float CurrentRecoilYaw = 0.0f;

	bool bIsFiring = false;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable)
	void RefillAmmo();

	UFUNCTION(BlueprintCallable)
	void GotExtraAmmo();

	UFUNCTION(BlueprintCallable)
	void Enhance();

private:
	int InitialReservedAmmo;
};
