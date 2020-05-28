	// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SPowerupActor.generated.h"

UCLASS()
class COOPGAME_API ASPowerupActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASPowerupActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Time between powerup ticks
	UPROPERTY(EditDefaultsOnly, Category="Powerups")
	float PowerupInterval;

	// Total time we apply the powerup effect
	UPROPERTY(EditDefaultsOnly, Category="Powerups")
	int32 TotalNoOfTicks;

	FTimerHandle TimerHandle_PowerupTick;

	// Total no of ticks applied
	int32 TicksProcessed;

	UFUNCTION()
	void OnTickPowerup();
	
public:	

	void ActivatePowerup();

	UFUNCTION(BlueprintImplementableEvent, Category="Powerups")
	void OnActivated();

	UFUNCTION(BlueprintImplementableEvent, Category="Powerups")
	void OnPowerUpTicked();

	UFUNCTION(BlueprintImplementableEvent, Category="Powerups")
	void OnExpired();
};
