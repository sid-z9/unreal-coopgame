// Fill out your copyright notice in the Description page of Project Settings.


#include "SGameMode.h"
#include "TimerManager.h"
#include "Components/SHealthComponent.h"
#include "Engine/World.h"

ASGameMode::ASGameMode()
{
    TimeBetweenWaves = 2.0f;

    PrimaryActorTick.bCanEverTick = true;    // by default in game mode, tick is disabled. Enabling it here.
    PrimaryActorTick.TickInterval = 1.0f;    // by default tick is refreshed every frame, changing here so it refreshes every second
}

void ASGameMode::StartWave()
{
    WaveCount++;

    NrOfBotsToSpawn = 2 * WaveCount;
    
    GetWorldTimerManager().SetTimer(TimerHandle_BotSpawnner, this, &ASGameMode::SpawnBotTimerElapsed,
        1.0f, true, 0.0f);
}

void ASGameMode::SpawnBotTimerElapsed()
{
    SpawnNewBot();

    NrOfBotsToSpawn--;

    if(NrOfBotsToSpawn <= 0)
    {
        EndWave();
    }
}

void ASGameMode::StartPlay()
{
    Super::StartPlay();

    PrepareForNextWave();
}

void ASGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    CheckWaveState();    // Every one second CheckWaveState() function will be run

    CheckAnyPlayerAlive();
}

void ASGameMode::PrepareForNextWave()
{   
    GetWorldTimerManager().SetTimer(TimerHandle_NextWaveStart, this, &ASGameMode::StartWave,
        TimeBetweenWaves, false);
}

void ASGameMode::CheckWaveState()
{
    bool bIsPreparingForWave = GetWorldTimerManager().IsTimerActive(TimerHandle_NextWaveStart);
    
    if(NrOfBotsToSpawn > 0 || bIsPreparingForWave)    // if there are still bots left to spawn and we dont want to prepare fot next wave if we are already preparing for next wave 
    {
        return;
    }

    bool bIsAnyBotAlive = false;
    
    for(FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)    // iterate over all pawns in scene
    {
        APawn* TestPawn = It->Get();
        if(TestPawn == nullptr || TestPawn->IsPlayerControlled())    // if player-controlled pawn found then continue
        {
            continue;;
        }

        USHealthComponent* HealthComp = Cast<USHealthComponent>(TestPawn->GetComponentByClass(USHealthComponent::StaticClass()));
        if(HealthComp && HealthComp->GetHealth() > 0.0f)    // a bot found with some health
        {
            bIsAnyBotAlive = true;
            break;
        }
    }

    if(!bIsAnyBotAlive)    // if all the bots are dead, then prepare for next wave
    {
        PrepareForNextWave();
    }
}

void ASGameMode::CheckAnyPlayerAlive()
{
    for(FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if(PC && PC->GetPawn())
        {
            APawn* MyPawn = PC->GetPawn();
            USHealthComponent* HealthComp = Cast<USHealthComponent>(
                MyPawn->GetComponentByClass(USHealthComponent::StaticClass()));

            if(ensure(HealthComp) && HealthComp->GetHealth() > 0.0f)     // ensure breaks the code(like a breakpoint) in case the check fails
            {
                // A Player is still alive
                return;
            }
        }
    }

    // No player alive
    GameOver();
}

void ASGameMode::GameOver()
{
    EndWave();

    UE_LOG(LogTemp, Warning, TEXT("Game Over! All Players Dead!"))
}

void ASGameMode::EndWave()
{
    GetWorldTimerManager().ClearTimer(TimerHandle_BotSpawnner);

   
}
