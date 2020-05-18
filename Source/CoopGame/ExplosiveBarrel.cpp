// Fill out your copyright notice in the Description page of Project Settings.


#include "ExplosiveBarrel.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AExplosiveBarrel::AExplosiveBarrel()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComp;
	MeshComp->SetSimulatePhysics(true);
	MeshComp->SetCollisionObjectType(ECC_PhysicsBody);	// set physics body, so that radial component can affect nearby barrels as well

	RadialForceComp = CreateDefaultSubobject<URadialForceComponent>(TEXT("RadialForceComponent"));
	RadialForceComp->Radius = 500;
	/*Prevent component from ticking, and only use FireImpulse() instead. If this is set to true,
	then the tick component of RadialForceComponent is executed at every frame*/
	RadialForceComp->bAutoActivate = false;
	RadialForceComp->bImpulseVelChange = true;
	RadialForceComp->bIgnoreOwningActor = true;		// does not apply impulse on the player 
	RadialForceComp->SetupAttachment(MeshComp);

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComponent"));

	ExplodedBarrelMaterial = CreateDefaultSubobject<UMaterialInterface>(TEXT("ExplodedBarrelMaterial"));
	
	bExploded = false;

	SetReplicates(true);

	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;
}

// Called when the game starts or when spawned
void AExplosiveBarrel::BeginPlay()
{
	Super::BeginPlay();

	HealthComp->OnHealthChanged.AddDynamic(this, &AExplosiveBarrel::OnHealthChanged);	
}

// Called every frame
void AExplosiveBarrel::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AExplosiveBarrel::OnHealthChanged(USHealthComponent* HealthComponent, float Health, float HealthData, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	if(Health <= 0.f && !bExploded)
	{
		bExploded = true;	

		PlayExplosionEffect();
	}
}

void AExplosiveBarrel::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AExplosiveBarrel, bExploded);	// replicate bExploded variable to any relevant client connected to server
}

void AExplosiveBarrel::OnRep_bExploded()
{
	PlayExplosionEffect();
}

void AExplosiveBarrel::PlayExplosionEffect()
{
	// Play Explosion Effect
	if(ExplosionEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());	
	}

	// Make the barrel jump
	MeshComp->AddImpulse(FVector(0, 0, 600), NAME_None, true);

	// Change the barrel material
	if(ExplodedBarrelMaterial)
	{
		MeshComp->SetMaterial(0, ExplodedBarrelMaterial);
	}
		
	// Push away nearby actors
	RadialForceComp->FireImpulse();
}
