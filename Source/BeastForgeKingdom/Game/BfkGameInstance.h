// BeastForge Kingdom — game instance: profile, run flow, battle hosting.
#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Core/BfkTypes.h"
#include "Battle/BfkBattle.h"
#include "Run/BfkRun.h"
#include "Meta/BfkProfile.h"
#include "BfkGameInstance.generated.h"

UENUM()
enum class EBfkBattleContext : uint8 { None, RunNode, LocalBattle, Pvp };

USTRUCT()
struct FBfkRewardBundle
{
	GENERATED_BODY()

	UPROPERTY() int32 Gold = 0;
	UPROPERTY() TArray<FName> CardChoices;      // pick one (or skip)
	UPROPERTY() FName RelicDrop;                // elite/boss
	UPROPERTY() FName WeaponDrop;
	UPROPERTY() int32 Emberglass = 0;
	UPROPERTY() int32 Forgedust = 0;
	UPROPERTY() TArray<FName> Captured;         // species captured this battle
	UPROPERTY() TArray<FBfkOwnedBeast> Hatched; // eggs that hatched
	UPROPERTY() TArray<FBfkMilestone> Milestones;
};

USTRUCT()
struct FBfkShopStock
{
	GENERATED_BODY()

	UPROPERTY() TArray<FName> Cards;
	UPROPERTY() TArray<int32> CardPrices;
	UPROPERTY() TArray<FName> Weapons;
	UPROPERTY() TArray<int32> WeaponPrices;
	UPROPERTY() TArray<FName> Relics;
	UPROPERTY() TArray<int32> RelicPrices;
	UPROPERTY() bool bHealPurchased = false;
};

UCLASS()
class UBfkGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	// ---- profile ----
	UBfkSaveGame* Profile();
	void SaveProfile();
	void ResetProfile();

	// ---- run flow ----
	bool HasActiveRun();
	void StartNewRun(const TArray<FBfkRunSquadMember>& Squad, int32 Seed);
	void AbandonRun();
	FBfkRunState& Run();
	TArray<int32> ReachableNodes();

	// Entering a node. Battle nodes stage a battle; other nodes are handled by
	// their screens which call the helpers below.
	const FBfkMapNode* CurrentNode();
	const FBfkMapNode* Node(int32 Id);
	void MoveToNode(int32 Id);

	// ---- battle hosting ----
	UBfkBattle* StartNodeBattle();
	UBfkBattle* StartLocalBattle(const TArray<FBfkRunSquadMember>& Squad, const TArray<FName>& EnemySpecies, EBfkWeather Weather, int32 Seed);
	UBfkBattle* StartPvpBattle(const TArray<FBfkRunSquadMember>& SquadA, const TArray<FBfkRunSquadMember>& SquadB, EBfkWeather Weather, int32 Seed);
	UBfkBattle* ActiveBattle() { return Battle; }
	EBfkBattleContext BattleContext() const { return BattleCtx; }

	// Called by the battle screen when the fight resolves. Returns rewards (run context).
	FBfkRewardBundle FinishBattle();

	// ---- rewards / shop / events / forge ----
	void TakeCardReward(FName Card);
	FBfkShopStock& ShopFor(const FBfkMapNode& NodeRef);
	bool BuyShopCard(int32 Index);
	bool BuyShopWeapon(int32 Index);
	bool BuyShopRelic(int32 Index);
	bool BuyShopHeal();
	bool ForgeUpgradeCard(FName Card);          // one upgrade per forge node
	bool RestHeal();                            // rest node: heal 30%
	bool RestHatch();                           // rest node: -1 egg timer (all eggs)

	// events
	FString ResolveEvent(EBfkEventKind Kind, int32 Choice); // returns outcome text

	// advancing acts
	bool AdvanceAct();                          // after boss; false = game won

	// ---- squad gear (during run) ----
	void EquipWeapon(int32 SquadIdx, FName Weapon);
	void EquipRelic(int32 SquadIdx, FName Relic);

	// ---- settings ----
	void ApplySettings();

	// deck the player battles with (species kits + extras + upgrades applied)
	TArray<FName> RunDeckExtras() { return Profile()->Run.ExtraCards; }

	// ui sound helper honoring volume settings
	void UiSound(FName Slug, float Volume = 1.f);
	float EffectiveSfxVolume();

private:
	UPROPERTY() UBfkSaveGame* Save = nullptr;
	UPROPERTY() UBfkBattle* Battle = nullptr;
	UPROPERTY() EBfkBattleContext BattleCtx = EBfkBattleContext::None;
	UPROPERTY() TMap<int32, FBfkShopStock> Shops;        // node id -> stock
	UPROPERTY() TSet<int32> ForgedNodes;                  // forge upgrade used
	UPROPERTY() TSet<int32> RestedNodes;
	int32 PendingNodeForBattle = -1;

	void ApplyBattleHpToSquad();
	static const TCHAR* SlotName() { return TEXT("BeastForgeProfile"); }
};
