// Fill out your copyright notice in the Description page of Project Settings.

// TODO: Swarm AI - Increase damage level when multiple tracker bots get nearby - 11.13

#include "STrackerBot.h"


#include "DrawDebugHelpers.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "./../Components/SHealthComponent.h"
#include "CoopGame/SCharacter.h"
#include "Components/SphereComponent.h"
#include "Sound/SoundCue.h"

// Sets default values
ASTrackerBot::ASTrackerBot()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetCanEverAffectNavigation(false);
	MeshComp->SetSimulatePhysics(true);
	RootComponent = MeshComp;

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));
	HealthComp->OnHealthChanged.AddDynamic(this, &ASTrackerBot::HandleTakeDamage);

	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->SetSphereRadius(200);
	SphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SphereComp->SetupAttachment(RootComponent);
	
	bUseVelocityChange = true;
	MovementForce = 1000;
	RequiredDistanceToTarget = 100;

	ExplosionDamage = 40;
	ExplosionRadius = 200;

	SelfDamageInterval = 0.25f;
}

// Called when the game starts or when spawned
void ASTrackerBot::BeginPlay()
{
	Super::BeginPlay();

	if(GetLocalRole() == ROLE_Authority)
	{
		// Find Initial Move-To
		NextPathPoint = GetNextPathPoint();	
	}
}

// Called every frame
void ASTrackerBot::Tick(float DeltaTime)
{
	if(GetLocalRole() == ROLE_Authority && !bExploded)
	{
		Super::Tick(DeltaTime);

		float DistanceToTarget = (GetActorLocation() - NextPathPoint).Size();

		if(DistanceToTarget <= RequiredDistanceToTarget)
		{
			NextPathPoint = GetNextPathPoint();

			DrawDebugString(GetWorld(), GetActorLocation(), "Target Reached");
		}
		else
		{
			// Keep moving towards the next target
			FVector ForceDirection = NextPathPoint - GetActorLocation();
			ForceDirection.Normalize();

			ForceDirection *= MovementForce;

			MeshComp->AddForce(ForceDirection, NAME_None, bUseVelocityChange);

			DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + ForceDirection, 32, FColor::Black, false, 0.0f, 0, 1.0f);
		}

		DrawDebugSphere(GetWorld(), NextPathPoint, 20, 12, FColor::Yellow, false, 0.0f, 1.0f);

		TArray<AActor*> OverlappingTrackerBots;

		// MeshComp->GetOverlappingActors(OverlappingTrackerBots);
		//
		// for (AActor* OverlappedActor : OverlappingTrackerBots)
		// {
		// 	UE_LOG(LogTemp, Warning, TEXT("%s overlapped"), *OverlappedActor->GetName());
		// }
	}
}

FVector ASTrackerBot::GetNextPathPoint()
{
	// Hack, to get player location. Will not work in multiplayer
	ACharacter* PlayerPawn = UGameplayStatics::GetPlayerCharacter(this, 0);
	
	UNavigationPath* NavPath = UNavigationSystemV1::FindPathToActorSynchronously(this, GetActorLocation(), PlayerPawn);

	if(NavPath && NavPath->PathPoints.Num() > 1) 	// 0th index points to the location where we currently are, 1st index points to the next location
	{
		// return next point in the path
		return NavPath->PathPoints[1];
	}

	// failed to find path
	return GetActorLocation();
}

void ASTrackerBot::HandleTakeDamage(USHealthComponent* HealthComponent, float Health, float HealthData,
    const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	// Pulse the material on hit
	if(MatInst == nullptr)
	{
		MatInst = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));
	}

	if(MatInst)
	{
		MatInst->SetScalarParameterValue("LastTimeDamageTaken", GetWorld()->TimeSeconds);
	}

	UE_LOG(LogTemp, Warning, TEXT("Health is %s, of %s"), *FString::SanitizeFloat(Health), *GetName());

	// Explode on hitpoints == 0
	if(Health <= 0.0f)
	{
		SelfDestruct();
	}
}

void ASTrackerBot::SelfDestruct()
{
	if(bExploded)
	{
		return;
	}
	bExploded = true;

	if(ExplosionEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());	
	}

	UGameplayStatics::PlaySoundAtLocation(this, ExplodeSound, GetActorLocation());

	MeshComp->SetVisibility(false, true);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	if(GetLocalRole() == ROLE_Authority)
	{
		TArray<AActor*> IgnoredActors;
		IgnoredActors.Add(this);	// Ignores damage on itself, however does not ignore other tracker bots

		// Apply Damage
		UGameplayStatics::ApplyRadialDamage(this, ExplosionDamage, GetActorLocation(), ExplosionRadius, nullptr, IgnoredActors, this, GetInstigatorController(), true);

		DrawDebugSphere(GetWorld(), GetActorLocation(), ExplosionRadius, 12, FColor::Red, false, 2.0f, 0, 2.0f);

		// Delete Actor Immediately
		// Destroy();	

		SetLifeSpan(2.0f); 	// give a buffer time amount so that the changes will be replicated on client and not call Destroy() immediately
	}	
}

void ASTrackerBot::DamageSelf()
{
	UGameplayStatics::ApplyDamage(this, 20, GetInstigatorController(), this, nullptr);
}

void ASTrackerBot::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	
	if(!bStartedSelfDestruction && !bExploded)
	{
		ASCharacter* PlayerPawn = Cast<ASCharacter>(OtherActor);
		if(PlayerPawn)
		{
			// We Overlapped a player

			if(GetLocalRole() == ROLE_Authority)
			{
				// Start Self Destruction Sequence
				GetWorldTimerManager().SetTimer(TimerHandle_SelfDamage, this, &ASTrackerBot::DamageSelf, SelfDamageInterval, true, 0.0f);	
			}
			
			bStartedSelfDestruction = true;

			UGameplayStatics::SpawnSoundAttached(SelfDestructSound, RootComponent);
		}

		ASTrackerBot* TrackerBot = Cast<ASTrackerBot>(OtherActor);
		if(TrackerBot)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s overlapped"), *GetName());	
		}
	}
}