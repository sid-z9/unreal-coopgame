// Fill out your copyright notice in the Description page of Project Settings.


#include "SWeapon.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "CoopGame.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

static int32 DebugWeaponDrawing = 0;
FAutoConsoleVariableRef CVARDebugWeaponDrawing (
	TEXT("COOP.DebugWeapons"),	// write this in console by pressing tilde(`) key
	DebugWeaponDrawing,			// the variable to use
	TEXT("Draw Debug Lines for Weapons"),
	ECVF_Cheat	// Enabled in cheat mode only
);

// Sets default values
ASWeapon::ASWeapon()
{
	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>("MeshComp");
	RootComponent = MeshComp;

	MuzzleSocketName = "MuzzleSocket";
	TracerTargetName = "Target";

	BaseDamage = 20.f;

	RateOfFire = 600.f;

	SetReplicates(true);	// when set to true, when server spawns this weapon, it will be replicated(spawned) on clients as well

	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;
}

void ASWeapon::BeginPlay()
{
	Super::BeginPlay();

	TimeBetweenShots = 60 / RateOfFire;
}

void ASWeapon::Fire()
{
	if(GetLocalRole() < ROLE_Authority)		
	{
		ServerFire();	// when client fires, the same has to be replicated on server side
	}

	// trace the world from pawn eyes to world location

	AActor* MyOwner = GetOwner();

	if(MyOwner)
	{
		FVector EyeLocation;
		FRotator EyeRotation;
		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		FVector ShotDirection = EyeRotation.Vector();

		FVector TraceEnd = EyeLocation + (ShotDirection * 10000);	// Since EyeRotaion is a FRotator, converting it to FVector and the trace end to a distance if 10000 units

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(MyOwner);
		QueryParams.AddIgnoredActor(this);
		QueryParams.bTraceComplex = true;
		QueryParams.bReturnPhysicalMaterial = true;

		// particle "Target" parameter
		FVector TracerEndPoint = TraceEnd;

		EPhysicalSurface SurfaceType = SurfaceType_Default;

		FHitResult Hit;
		if(GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, TraceEnd, COLLISION_WEAPON, QueryParams))	// bullet trace
		{
			// Blocking Hit, Process Damage
			AActor* HitActor = Hit.GetActor();

			SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());
			
			float ActualDamage = BaseDamage;
			if(SurfaceType == SURFACE_FLESH_VULNERABLE)
			{
				ActualDamage *= 4.f;
			}
			
			UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, ShotDirection, Hit, MyOwner->GetInstigatorController(), this, DamageType);

			PlayImpactEffects(SurfaceType, Hit.ImpactPoint);

			// update the TraceEndPoint to the impact point
			TracerEndPoint = Hit.ImpactPoint;
		}	

		if(DebugWeaponDrawing > 0)
		{
			DrawDebugLine(GetWorld(), EyeLocation, TraceEnd, FColor::Red, false, 5.0f, 0, 1.0f);
		}

		PlayFireEffects(TracerEndPoint);

		if(GetLocalRole() == ROLE_Authority)
		{
			HitScanTrace.TraceTo = TracerEndPoint;
			HitScanTrace.SurfaceType = SurfaceType;
		}
		
		LastFireTime = GetWorld()->TimeSeconds;
	}
}

void ASWeapon::PlayFireEffects(FVector TraceEnd)
{
	if(MuzzleEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocketName);
	}

	if(TracerEffect)
	{
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		UParticleSystemComponent* TracerComp = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TracerEffect, MuzzleLocation);

		if(TracerComp)
		{
			TracerComp->SetVectorParameter(TracerTargetName, TraceEnd);
		}
	}

	APawn* MyOwner = Cast<APawn>(GetOwner());
	if(MyOwner)
	{
		APlayerController* PC = Cast<APlayerController>(MyOwner->GetController());
		if(PC)
		{
			PC->ClientPlayCameraShake(FireCamShake);
		}
	}
}

void ASWeapon::StartFire()
{
	float FirstDelay = FMath::Max(LastFireTime + TimeBetweenShots - GetWorld()->TimeSeconds, 0.f);

	GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenShots, this, &ASWeapon::Fire, TimeBetweenShots, true, FirstDelay);
}

void ASWeapon::StopFire()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);
}

void ASWeapon::ServerFire_Implementation()	// when dealing with server functions, prefix them with server and underscore implementation
{
	Fire();
}

bool ASWeapon::ServerFire_Validate()	// since we used WithValidation keyword, have to specify this function; its used for anti-cheat
{
	return true;		// this function is used to validate code, if returned false, the client will be disconnected from the server
}

void ASWeapon::OnRep_HitScanTrace()
{
	// play cosmetic effects
	PlayFireEffects(HitScanTrace.TraceTo);
	PlayImpactEffects(HitScanTrace.SurfaceType, HitScanTrace.TraceTo);
}

void ASWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// replicate HitScanTrace variable to any relevant client connected to server
	DOREPLIFETIME_CONDITION(ASWeapon, HitScanTrace, COND_SkipOwner);	// the condition is specified so that the owning client does not replicate HitScanTrace again; ie replicates to all clients except the owning client
}


void ASWeapon::PlayImpactEffects(EPhysicalSurface SurfaceType, FVector ImpactPoint)
{
	UParticleSystem* SelectedEffect = nullptr;
	switch(SurfaceType)
	{
	case SURFACE_FLESH_DEFAULT:
    case SURFACE_FLESH_VULNERABLE:
        SelectedEffect = FleshImpactEffect;
		break;
	default:
        SelectedEffect = DefaultImpactEffect;
	}
	if(SelectedEffect)
	{
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		FVector ShotDirection = ImpactPoint - MuzzleLocation;
		ShotDirection.Normalize();
		
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, ImpactPoint, ShotDirection.Rotation());
	}	
}
