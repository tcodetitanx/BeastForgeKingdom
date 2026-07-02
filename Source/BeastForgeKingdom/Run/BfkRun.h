// BeastForge Kingdom — roguelite run state: act maps, node resolution, rewards.
#pragma once

#include "CoreMinimal.h"
#include "Core/BfkTypes.h"
#include "Battle/BfkBattle.h"
#include "BfkRun.generated.h"

UENUM()
enum class EBfkEventKind : uint8
{
	WildEgg,        // gamble emberglass for an egg
	CursedShrine,   // relic for max-hp price
	Castaway,       // free card or gold
	RainTotem,      // change next battles' weather
	Forgefire,      // upgrade a card
	TollBridge      // pay gold or lose hp
};

USTRUCT()
struct FBfkRunSquadMember
{
	GENERATED_BODY()

	UPROPERTY() FGuid VaultId;             // which owned beast/hero
	UPROPERTY() FName Species;
	UPROPERTY() FName Weapon;
	UPROPERTY() FName Relic;
	UPROPERTY() int32 CurrentHp = -1;      // persistent across battles; -1 = full
};

USTRUCT()
struct FBfkRunState
{
	GENERATED_BODY()

	UPROPERTY() bool bActive = false;
	UPROPERTY() int32 Seed = 0;
	UPROPERTY() int32 Act = 1;
	UPROPERTY() int32 NodeId = -1;               // current node (-1 = start)
	UPROPERTY() TArray<FBfkMapNode> Map;         // current act's map
	UPROPERTY() TArray<FBfkRunSquadMember> Squad;
	UPROPERTY() TArray<FName> ExtraCards;        // cards drafted this run (join deck)
	UPROPERTY() TMap<FName, int32> UpgradedCards;// slug -> upgraded copies count
	UPROPERTY() int32 Gold = 60;
	UPROPERTY() int32 BattlesWon = 0;
	UPROPERTY() int32 ElitesSlain = 0;
	UPROPERTY() int32 CapturesThisRun = 0;
	UPROPERTY() bool bBoonPending = false;       // pick a starting boon on the first map visit
	UPROPERTY() bool bBossDefeated = false;
	UPROPERTY() EBfkWeather ForcedWeather = EBfkWeather::Gloom;
	UPROPERTY() bool bHasForcedWeather = false;
};

class FBfkRunLogic
{
public:
	// Generates a Slay-the-Spire style layered map for the given act.
	static TArray<FBfkMapNode> GenerateMap(int32 Seed, int32 Act);

	// Nodes reachable from the current node.
	static TArray<int32> ReachableNodes(const FBfkRunState& Run);

	// Battle config for a node (Battle/Elite/Boss).
	static FBfkBattleConfig MakeBattleConfig(const FBfkRunState& Run, const FBfkMapNode& Node,
	                                         const TArray<FName>& RunDeckExtras);

	// Rewards after victory.
	static int32 GoldReward(EBfkNode Type, FRandomStream& Rng);

	// Event for an event node (deterministic by node seed).
	static EBfkEventKind EventFor(const FBfkMapNode& Node);
};
