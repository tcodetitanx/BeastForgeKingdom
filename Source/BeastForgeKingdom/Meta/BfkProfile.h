// BeastForge Kingdom — permanent profile: vault, breeding, unlocks, settings.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Core/BfkTypes.h"
#include "Run/BfkRun.h"
#include "BfkProfile.generated.h"

USTRUCT()
struct FBfkMilestone
{
	GENERATED_BODY()

	UPROPERTY() FName Id;
	UPROPERTY() FString Display;
	UPROPERTY() FString Desc;
	UPROPERTY() FName RewardRelic;
	UPROPERTY() FName RewardWeapon;
	UPROPERTY() int32 RewardShards = 0;
};

USTRUCT()
struct FBfkSettings
{
	GENERATED_BODY()

	UPROPERTY() float MasterVolume = 0.8f;
	UPROPERTY() float SfxVolume = 1.0f;
	UPROPERTY() float MusicVolume = 0.7f;
	UPROPERTY() int32 WeatherIntensity = 2;    // 0 off, 1 light, 2 full
	UPROPERTY() bool bScreenShake = true;
	UPROPERTY() bool bFastAnimations = false;
	UPROPERTY() int32 WindowMode = 0;          // 0 windowed, 1 borderless, 2 fullscreen
};

UCLASS()
class UBfkSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY() int32 Version = 1;

	// vault
	UPROPERTY() TArray<FBfkOwnedBeast> Vault;
	UPROPERTY() TArray<FBfkEgg> Eggs;

	// currencies
	UPROPERTY() int32 Soulshards = 0;
	UPROPERTY() int32 Emberglass = 3;
	UPROPERTY() int32 Forgedust = 0;   // permanent; levels up beasts (breeding's impatient cousin)

	// unlocks & records
	UPROPERTY() TSet<FName> UnlockedWeapons;      // weapons available in pools
	UPROPERTY() TSet<FName> UnlockedRelics;
	UPROPERTY() TSet<FName> CompletedMilestones;
	UPROPERTY() TSet<FName> SeenEnemies;          // enemy library
	UPROPERTY() TSet<FName> OwnedWeapons;         // armory (equippable)
	UPROPERTY() TSet<FName> OwnedRelics;
	UPROPERTY() int32 RunsStarted = 0;
	UPROPERTY() int32 RunsWon = 0;
	UPROPERTY() int32 DeepestAct = 1;
	UPROPERTY() int32 TotalCaptures = 0;
	UPROPERTY() int32 TotalBreeds = 0;

	// active run (resumable)
	UPROPERTY() FBfkRunState Run;

	UPROPERTY() FBfkSettings Settings;
	UPROPERTY() bool bSeenIntro = false;
};

class FBfkMeta
{
public:
	static const TArray<FBfkMilestone>& Milestones();

	// Grants starter content on a fresh profile.
	static void InitFreshProfile(UBfkSaveGame& Save);

	// Vault helpers.
	static FBfkOwnedBeast* FindBeast(UBfkSaveGame& Save, const FGuid& Id);
	static FBfkOwnedBeast MakeBeast(FName Species, int32 Generation = 0);

	// Breeding: consumes emberglass, returns egg (or nullopt-like invalid guid).
	static bool CanBreed(const UBfkSaveGame& Save, const FGuid& A, const FGuid& B, FString& WhyNot);
	static FBfkEgg Breed(UBfkSaveGame& Save, const FGuid& A, const FGuid& B, int32 Seed);
	static int32 BreedCost() { return 2; }

	// After each battle victory, eggs tick toward hatching.
	static TArray<FBfkOwnedBeast> TickEggs(UBfkSaveGame& Save);

	// Forgedust leveling: permanent stat growth without breeding.
	static int32 LevelUpCost(int32 CurrentLevel) { return 25 + CurrentLevel * 15; }
	static bool LevelUpBeast(UBfkSaveGame& Save, const FGuid& Id, FString& WhyNot);

	// Milestones: evaluates and returns newly completed ones.
	static TArray<FBfkMilestone> EvaluateMilestones(UBfkSaveGame& Save);
};
