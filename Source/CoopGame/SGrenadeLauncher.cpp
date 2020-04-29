// Fill out your copyright notice in the Description page of Project Settings.


#include "SGrenadeLauncher.h"
// #include "ASProjectile"

// ASGrenadeLauncher::ASGrenadeLauncher()
// {
//     // Super::ASWeapon();

    

//     // MuzzleSocketName = "MuzzleSocket";
// 	// TracerTargetName = "Target";
// }

// // Function that initializes the projectile's velocity in the shoot direction.
// void ASGrenadeLauncher::FireInDirection(const FVector& ShootDirection)
// {
//     ProjectileMovementComponent->Velocity = ShootDirection * ProjectileMovementComponent->InitialSpeed;
// }

// virtual void Fire() override
// {
//     AActor* MyOwner = GetOwner();

    

//     if(MyOwner)
//     {
//         // FVector EyeLocation;
// 		// FRotator EyeRotation;
// 		// MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

//         FActorSpawnParameters SpawnParams;
//         SpawnParams.Owner = this;
//         SpawnParams.Instigator = Instigator;

//         FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
//         FRotator MuzzleRotation = MuzzleLocation.Rotation();

        

//         ASProjectile* Projectile = World->SpawnActor<ASProjectile>(ProjectileClass, MuzzleLocation, MuzzleRotation, SpawnParams);
//         if(Projectile)
//         {
//             FVector ShotDirection = MuzzleLocation.Vector();
//             FireInDirection(ShotDirection);
//         }
//     }
// }

