// BeastForge Kingdom — deterministic 3v3 positional battle model.
// Pure simulation: no widgets, no actors. The UI replays the event queue.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Core/BfkTypes.h"
#include "BfkBattle.generated.h"

// ------------------------------------------------------------- Units

USTRUCT()
struct FBfkUnitState
{
	GENERATED_BODY()

	UPROPERTY() int32 Id = -1;
	UPROPERTY() FName Species;
	UPROPERTY() FString Display;
	UPROPERTY() bool bEnemySide = false;
	UPROPERTY() FBfkCell Cell;
	UPROPERTY() int32 MaxHp = 30;
	UPROPERTY() int32 Hp = 30;
	UPROPERTY() int32 Block = 0;
	UPROPERTY() int32 Power = 0;
	UPROPERTY() TMap<EBfkStatus, int32> Statuses;
	UPROPERTY() FName Weapon;
	UPROPERTY() FName Relic;
	UPROPERTY() bool bAlive = true;
	UPROPERTY() bool bBoss = false;
	UPROPERTY() bool bBeast = true;
	UPROPERTY() bool bOriginal = true;   // false = summoned mid-battle reinforcement
	UPROPERTY() FGuid VaultId;            // ally: link back to owned beast
	UPROPERTY() int32 Range = 1;          // attack reach in hexes
	UPROPERTY() int32 Move = 2;           // voluntary move speed in hexes/turn
	UPROPERTY() bool bMovedThisTurn = false;

	int32 Status(EBfkStatus S) const { const int32* V = Statuses.Find(S); return V ? *V : 0; }
};

// ------------------------------------------------------------- Cards in play

USTRUCT()
struct FBfkCardInstance
{
	GENERATED_BODY()

	UPROPERTY() int32 InstanceId = -1;
	UPROPERTY() FName CardSlug;
	UPROPERTY() int32 OwnerUnitId = -1;   // severed while owner is dead
	UPROPERTY() bool bUpgraded = false;
	UPROPERTY() bool bToken = false;      // battle-only (Soul Snare, curses)
};

// ------------------------------------------------------------- Events

UENUM()
enum class EBfkEvt : uint8
{
	TurnBegan,          // A=turn number, B=side (0 player / 1 enemy or P2)
	EnergyChanged,      // A=new energy, B=side
	CardDrawn,          // A=instance id, B=side
	CardPlayed,         // A=instance id, Slug=card, Unit=owner
	CardDiscarded,      // A=instance id, B=side
	CardExhausted,      // A=instance id
	CardSevered,        // A=instance id (owner died)
	CardRestored,       // A=instance id
	DeckShuffled,       // B=side
	UnitAttack,         // Unit=attacker, Unit2=target (lunge anim), Slug=card
	UnitDamaged,        // Unit=victim, A=hp lost, B=blocked amount
	UnitBlocked,        // Unit, A=block gained
	UnitHealed,         // Unit, A
	StatusApplied,      // Unit, A=stacks (may be negative=removed), B=(int)status
	UnitMoved,          // Unit, A=fromRow*10+fromCol, B=toRow*10+toCol, C=1 if shoved
	Collision,          // Unit, Unit2 (or -1 = wall), A=dmg
	HazardChanged,      // A=row*10+col, B=(int)hazard (None=cleared)
	HazardTriggered,    // Unit, A=row*10+col, B=(int)hazard
	UnitDied,           // Unit
	UnitSummoned,       // Unit (new unit id; state already in Units)
	IntentUpdated,      // Unit, Slug=card, A=target row*10+col or -1
	ProjectileFly,      // Unit=source, A=target cell code, Slug=projectile sprite
	ParticleBurst,      // A=cell code, Slug=particle sprite slug
	Speech,             // Unit (or -1), Str=line
	WeatherPulse,       // A=(int)weather (ambient tick for UI)
	TideWarning,        // A=row (Tidebound King)
	TideFlood,          // A=row
	ColumnsCapsized,    // Ghostwake: ally cols swapped
	CaptureResult,      // Unit=target, A=1 success/0 fail, B=chance pct
	BattleEnded         // A=1 victory/0 defeat, B=side that won (pvp)
};

USTRUCT()
struct FBfkBattleEvent
{
	GENERATED_BODY()

	UPROPERTY() EBfkEvt Type = EBfkEvt::TurnBegan;
	UPROPERTY() int32 Unit = -1;
	UPROPERTY() int32 Unit2 = -1;
	UPROPERTY() int32 A = 0;
	UPROPERTY() int32 B = 0;
	UPROPERTY() int32 C = 0;
	UPROPERTY() FName Slug;
	UPROPERTY() FString Str;
};

// ------------------------------------------------------------- Config

USTRUCT()
struct FBfkFighterSpec
{
	GENERATED_BODY()

	UPROPERTY() FName Species;
	UPROPERTY() FString Display;          // override (nickname); empty = species
	UPROPERTY() int32 BonusHp = 0;
	UPROPERTY() int32 BonusPower = 0;
	UPROPERTY() FName InheritedCard;
	UPROPERTY() FName Weapon;
	UPROPERTY() FName Relic;
	UPROPERTY() FGuid VaultId;
};

USTRUCT()
struct FBfkBattleConfig
{
	GENERATED_BODY()

	UPROPERTY() TArray<FBfkFighterSpec> Allies;    // up to 3
	UPROPERTY() TArray<FBfkFighterSpec> Enemies;   // up to 3
	UPROPERTY() EBfkWeather Weather = EBfkWeather::Gloom;
	UPROPERTY() int32 Seed = 1337;
	UPROPERTY() bool bPvp = false;                 // side B is a human hand
	UPROPERTY() bool bAllowCapture = true;
	UPROPERTY() int32 Act = 1;
};

// ------------------------------------------------------------- Battle

UCLASS()
class UBfkBattle : public UObject
{
	GENERATED_BODY()

public:
	void Init(const FBfkBattleConfig& InConfig);

	// --- queries -----------------------------------------------------------
	const TArray<FBfkUnitState>& GetUnits() const { return Units; }
	const FBfkUnitState* FindUnit(int32 Id) const;
	const FBfkUnitState* UnitAt(const FBfkCell& Cell) const;
	EBfkHazard HazardAt(const FBfkCell& Cell) const;
	int32 GetEnergy(int32 Side) const { return Energy[Side]; }
	int32 GetTurn() const { return Turn; }
	int32 GetActiveSide() const { return ActiveSide; }
	bool IsOver() const { return bOver; }
	bool WasVictory() const { return bVictory; }
	EBfkWeather GetWeather() const { return Config.Weather; }
	const FBfkBattleConfig& GetConfig() const { return Config; }
	const TArray<FBfkCardInstance>& GetHand(int32 Side) const { return Hand[Side]; }
	const TArray<FBfkCardInstance>& GetDrawPile(int32 Side) const { return DrawPile[Side]; }
	const TArray<FBfkCardInstance>& GetDiscardPile(int32 Side) const { return Discard[Side]; }
	int32 GetDrawCount(int32 Side) const { return DrawPile[Side].Num(); }
	int32 GetDiscardCount(int32 Side) const { return Discard[Side].Num(); }
	const FBfkCardInstance* FindCard(int32 InstanceId) const;
	bool IsCardSevered(const FBfkCardInstance& CI) const;
	// Enemy intent for UI: card + chosen target cell (row*10+col, -1 none).
	// bOutAdvance: unit will move toward the player instead of acting.
	bool GetIntent(int32 UnitId, FName& OutCard, int32& OutTargetCode, bool& bOutAdvance) const;
	TArray<FName> CapturedThisBattle;

	// Valid targets for a card in hand (unit ids or cell codes depending on target kind)
	bool CanPlayCard(int32 InstanceId, FString& WhyNot) const;
	TArray<int32> GetValidUnitTargets(int32 InstanceId) const;
	TArray<int32> GetValidCellTargets(int32 InstanceId) const;   // row*10+col

	// voluntary movement: each unit may reposition once per its side's turn
	bool CanUnitMove(int32 UnitId) const;
	TArray<int32> GetMoveCells(int32 UnitId) const;              // row*10+col
	bool MoveCommand(int32 UnitId, int32 CellCode);

	// --- commands ----------------------------------------------------------
	// TargetCode: unit id for unit targets, row*10+col for cell/lane (-1 auto)
	bool PlayCard(int32 InstanceId, int32 TargetCode);
	void EndTurn(bool bKeepHand = false);   // keep = hand carries over; draw tops up to 5

	// Presentation drains events from here.
	TArray<FBfkBattleEvent> Events;

private:
	// state
	UPROPERTY() FBfkBattleConfig Config;
	UPROPERTY() TArray<FBfkUnitState> Units;
	UPROPERTY() TMap<FBfkCell, EBfkHazard> Hazards;
	UPROPERTY() int32 Turn = 0;
	UPROPERTY() int32 ActiveSide = 0;     // 0 = player/A, 1 = enemy/B
	int32 Energy[2] = {3, 3};
	int32 MaxEnergy[2] = {3, 3};
	int32 CarryEnergy[2] = {0, 0};   // unspent energy banked for next turn (capped)
	TArray<FBfkCardInstance> DrawPile[2];
	TArray<FBfkCardInstance> Hand[2];
	TArray<FBfkCardInstance> Discard[2];
	TArray<FBfkCardInstance> Exhausted[2];
	UPROPERTY() bool bOver = false;
	UPROPERTY() bool bVictory = false;
	FRandomStream Rng;
	int32 NextUnitId = 1;
	int32 NextCardId = 1;
	int32 SnareCharges = 1;

	// enemy intents (campaign mode): unit id -> (card slug, target code)
	struct FIntent { FName Card; int32 TargetCode = -1; bool bAdvance = false; };
	TMap<int32, FIntent> Intents;

	// boss bookkeeping
	int32 TideCounter = 0;
	int32 TideRow = -1;
	bool bFacetReflectArmed = false;
	int32 CapsizeCounter = 0;

	// --- internals ---------------------------------------------------------
	FBfkUnitState* MutFind(int32 Id);
	FBfkUnitState* MutUnitAt(const FBfkCell& Cell);
	void Emit(EBfkEvt Type, int32 Unit = -1, int32 Unit2 = -1, int32 A = 0, int32 B = 0,
	          int32 C = 0, FName Slug = NAME_None, const FString& Str = FString());
	static int32 CellCode(const FBfkCell& C) { return C.Row * 10 + C.Col; }
	static FBfkCell CodeCell(int32 Code) { return FBfkCell(Code / 10, Code % 10); }

	void BuildSide(const TArray<FBfkFighterSpec>& Specs, bool bEnemy);
	void BuildDeck(int32 Side);
	void ShuffleDraw(int32 Side);
	void DrawCards(int32 Side, int32 N);
	void StartTurn(int32 Side);
	void ResolveEnemyTurn();      // campaign AI side
	void PickIntents();
	void TickStatusesTurnStart(int32 Side);
	void TickStatusesTurnEnd(int32 Side);
	void TickHazardsFor(FBfkUnitState& U);
	void ApplyWeatherTurnStart(int32 Side);
	void BossTurnStart();
	void RelicHook(EBfkRelicHook Hook, FBfkUnitState& Holder, int32 Magnitude = 0);
	void RelicHooksSide(EBfkRelicHook Hook, int32 Side, int32 Magnitude = 0);

	void ResolveCard(const FBfkCardInstance& CI, const FBfkCardDef& Def, int32 TargetCode, FBfkUnitState& Owner);
	void ApplyEffect(const FBfkEffect& Fx, FBfkUnitState& Owner, int32 TargetCode, const FBfkCardDef& Def);
	int32 AttackDamage(const FBfkUnitState& Attacker, int32 Base) const;
	void DealDamage(FBfkUnitState* Attacker, FBfkUnitState& Victim, int32 Amount, bool bIsAttack, FName CardSlug = NAME_None);
	void ApplyStatus(FBfkUnitState& U, EBfkStatus S, int32 Stacks);
	void HealUnit(FBfkUnitState& U, int32 Amount);
	void GainBlock(FBfkUnitState& U, int32 Amount);
	void MoveUnit(FBfkUnitState& U, const FBfkCell& To, bool bShoved);
	void ShoveUnit(FBfkUnitState& Shover, FBfkUnitState& Target, int32 DeltaCol, int32 DeltaRow, int32 Dist);
	void PlaceHazard(const FBfkCell& Cell, EBfkHazard H);
	void KillUnit(FBfkUnitState& U, FBfkUnitState* Killer);
	void SeverCards(int32 UnitId);
	void RestoreCards(int32 UnitId);
	void CheckEnd();
	void SummonMinion(FName Species, bool bEnemySide, const FBfkCell& Prefer);
	FBfkUnitState* FirstInLane(int32 Row, bool bEnemySide, bool bFromAllyView);
	TArray<int32> SideUnitIds(bool bEnemy, bool bAliveOnly = true) const;
	const FBfkCardDef* Def(FName Slug) const;
	int32 CountAlive(bool bEnemy) const;
	FBfkCell FindFreeCell(bool bEnemySide, int32 PreferRow);
	void EnemyPlayIntent(FBfkUnitState& Enemy, const FIntent& Intent);
	void EnemyAdvance(FBfkUnitState& Enemy);              // greedy step toward nearest player unit
	const FBfkUnitState* NearestFoe(const FBfkUnitState& U) const;
};
