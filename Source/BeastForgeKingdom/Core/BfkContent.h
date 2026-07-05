// BeastForge Kingdom — static content registries (species, cards, gear, encounters).
// Hand-authored content lives in BfkContentCore.cpp; sprite-derived species/gear
// tables are generated from the art manifest into BfkContentGenerated.cpp.
#pragma once

#include "CoreMinimal.h"
#include "Core/BfkTypes.h"

class FBfkContent
{
public:
	static void EnsureInit();

	static const TMap<FName, FBfkSpeciesDef>& AllSpecies();
	static const TMap<FName, FBfkCardDef>& AllCards();
	static const TMap<FName, FBfkRelicDef>& AllRelics();
	static const TMap<FName, FBfkWeaponDef>& AllWeapons();

	static const FBfkSpeciesDef* Species(FName Slug);
	static const FBfkCardDef* Card(FName Slug);
	static const FBfkRelicDef* Relic(FName Slug);
	static const FBfkWeaponDef* Weapon(FName Slug);

	// Full card list for a fighter: signature + weapon grants + inherited.
	static TArray<FName> DeckFor(const struct FBfkFighterSpec& Spec);

	// Reward pool: cards drawn only from the squad's species pools + neutrals.
	static TArray<FName> CardRewardChoices(const TArray<FName>& SquadSpecies, FRandomStream& Rng, int32 Count, bool bElite);

	// Campaign tables.
	static TArray<FName> RollEncounter(int32 Act, bool bElite, FRandomStream& Rng);
	static FName BossFor(int32 Act);
	static TArray<FName> BossMinions(FName BossSlug);
	static FString ActName(int32 Act);
	static FString ActBackdrop(int32 Act);              // texture slug
	static EBfkWeather RollWeather(int32 Act, FRandomStream& Rng);

	// Pools for shops/rewards.
	static TArray<FName> RelicPool(FRandomStream& Rng, int32 Count, const TSet<FName>& Exclude);
	static TArray<FName> WeaponPool(FRandomStream& Rng, int32 Count, int32 Act, const TSet<FName>& Exclude);
	static TArray<FName> StarterSquad();                // first-run squad
	static TArray<FName> CapturableSpecies(int32 Act, FRandomStream& Rng, int32 Count);

	// Breeding: result species for two parents (element/archetype blend).
	static FName BreedResult(FName ParentA, FName ParentB, FRandomStream& Rng);

	// registration helpers (used by both content translation units)
	static void AddSpecies(const FBfkSpeciesDef& S);
	static void AddCard(const FBfkCardDef& C);
	static void AddRelic(const FBfkRelicDef& R);
	static void AddWeapon(const FBfkWeaponDef& W);

private:
	static void InitCore();       // BfkContentCore.cpp
	static void InitGenerated();  // BfkContentGenerated.cpp
	static void InitDerived();    // template cards for species without hand-authored kits
	static void InitLineup();     // translate positional ops for the lineup battle system
	static bool bInit;
};
