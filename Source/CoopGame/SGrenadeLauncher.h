// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SWeapon.h"
#include "SGrenadeLauncher.generated.h"

/**
 * 
 */

class ASProjectile2;

UCLASS()
class COOPGAME_API ASGrenadeLauncher : public ASWeapon
{
	GENERATED_BODY()

protected:
	// Projectile class to spawn.
   	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	TSubclassOf<ASProjectile2> ProjectileClass;

	// UFUNCTION(BlueprintCallable, Category = "Weapons")
	virtual void Fire() override;

	// void FireInDirection(const FVector& ShootDirection);
};
