// BeastForge Kingdom — core shared types.
#pragma once

#include "CoreMinimal.h"
#include "BfkTypes.generated.h"

// ---------------------------------------------------------------- Elements

UENUM()
enum class EBfkElement : uint8
{
	Ember, Frost, Verdant, Storm, Shadow, Iron, Arcane, Neutral
};

UENUM()
enum class EBfkArchetype : uint8
{
	Bruiser, Tank, Striker, Caster, Support, Trickster
};

UENUM()
enum class EBfkStatus : uint8
{
	Burn,     // N damage at turn start, decays by 1
	Chill,    // -1 damage dealt per attack while any; decays; 3+ freezes 1 energy
	Poison,   // N damage at turn end, then decays by 1
	Shock,    // next hit taken +N and chains N/2 to adjacent unit; consumed
	Curse,    // healing blocked while present; decays
	Rust,     // block gained reduced by N; decays
	Ward,     // block that persists between turns (consumed by damage)
	Rally,    // +N damage dealt per attack; decays
	Thorns,   // melee attackers take N
	Stealth   // untargetable by single-target attacks for N turns
};

UENUM()
enum class EBfkWeather : uint8
{
	Gloom,    // baseline dark; no modifier
	Rain,     // ember -1 dmg, storm +1 dmg; puddles spawn; shock chains +1
	Snow,     // frost +1 dmg; exposed back column gains 1 chill each 2 turns
	Ashfall   // shadow +1 dmg; all healing -1
};

UENUM()
enum class EBfkHazard : uint8
{
	None,
	Coals,        // entering/starting here: 2 burn
	Hoarfrost,    // entering/starting here: 2 chill
	VoidRift,     // entering/starting here: 1 curse; +1 shadow dmg taken here
	PowderKeg,    // unit shoved into keg cell: 8 dmg explosion, keg cleared
	Puddle,       // rain; shock applied here chains to ALL puddle units
	Spores,       // 2 poison at turn start; spreads to one adjacent cell/turn (boss)
	CrystalShard, // entering: 3 dmg; attacking from here: +2 dmg (risk/reward)
	Floodmark     // tide warning: this cell floods at next tide (boss)
};

// ---------------------------------------------------------------- Cards

UENUM()
enum class EBfkCardKind : uint8 { Attack, Skill, Power, Curse };

UENUM()
enum class EBfkRarity : uint8 { Starter, Common, Uncommon, Rare, Legendary };

// What the player must pick when playing the card.
UENUM()
enum class EBfkTarget : uint8
{
	None,        // no pick: self / auto / whole side
	Enemy,       // any living enemy unit
	EnemyFront,  // any enemy currently reachable in melee (front-most in its lane)
	Ally,        // any living ally unit (may include self)
	AllySlot,    // any ally-side cell (summons / repositioning)
	Lane,        // one of 3 lanes
	Cell         // any board cell
};

UENUM()
enum class EBfkOp : uint8
{
	Damage,        // A dmg to target unit
	DamageLane,    // A dmg to first enemy in target lane
	DamageRow,     // A dmg to every enemy in target lane
	DamageAll,     // A dmg to all enemies
	DamageSelf,    // A dmg to the card's owner (costs/downsides)
	Block,         // A block to target ally (or owner if None)
	BlockAll,      // A block to all allies
	Heal,          // A hp to target ally (or owner)
	HealAll,
	Status,        // apply StatusA x A to target
	StatusAll,     // apply to all enemies
	StatusSelfSide,// apply to all allies
	Cleanse,       // remove A debuff stacks from target ally
	Push,          // shove target enemy A cells away from attacker (collision dmg)
	Pull,          // pull target enemy A cells toward attacker
	SwapAlly,      // swap owner with target ally
	MoveSelf,      // move owner to target ally-side cell
	Hazard,        // place HazardA at target cell
	ClearHazard,   // clear hazard at target cell
	Draw,          // draw A cards
	Energy,        // gain A energy this turn
	ExhaustSelf,   // this card exhausts (marker op)
	Retain,        // marker: not discarded at end of turn
	Capture,       // attempt soul-snare capture on target beast enemy
	SummonMinion   // enemy-only: summon species SlugParam at target/first-free cell
};

USTRUCT()
struct FBfkEffect
{
	GENERATED_BODY()

	UPROPERTY() EBfkOp Op = EBfkOp::Damage;
	UPROPERTY() int32 A = 0;                          // magnitude
	UPROPERTY() EBfkStatus StatusA = EBfkStatus::Burn;
	UPROPERTY() EBfkHazard HazardA = EBfkHazard::None;
	UPROPERTY() FName SlugParam;                       // summons etc.

	FBfkEffect() {}
	FBfkEffect(EBfkOp InOp, int32 InA) : Op(InOp), A(InA) {}
	FBfkEffect(EBfkOp InOp, int32 InA, EBfkStatus InStatus) : Op(InOp), A(InA), StatusA(InStatus) {}
	static FBfkEffect Dmg(int32 N) { return FBfkEffect(EBfkOp::Damage, N); }
	static FBfkEffect Blk(int32 N) { return FBfkEffect(EBfkOp::Block, N); }
	static FBfkEffect St(EBfkStatus S, int32 N) { FBfkEffect E(EBfkOp::Status, N); E.StatusA = S; return E; }
	static FBfkEffect Hz(EBfkHazard H) { FBfkEffect E(EBfkOp::Hazard, 1); E.HazardA = H; return E; }
};

USTRUCT()
struct FBfkCardDef
{
	GENERATED_BODY()

	UPROPERTY() FName Slug;
	UPROPERTY() FString Display;
	UPROPERTY() FString Text;             // rules text; {A0} etc substituted
	UPROPERTY() int32 Cost = 1;
	UPROPERTY() EBfkCardKind Kind = EBfkCardKind::Attack;
	UPROPERTY() EBfkRarity Rarity = EBfkRarity::Common;
	UPROPERTY() EBfkElement Element = EBfkElement::Neutral;
	UPROPERTY() EBfkTarget Target = EBfkTarget::Enemy;
	UPROPERTY() bool bMeleeOnly = false;  // owner must be in front column to play
	UPROPERTY() TArray<FBfkEffect> Effects;
	UPROPERTY() TArray<FBfkEffect> UpgradedEffects;   // empty = +25% A on all
	UPROPERTY() FName ArtSlug;            // card-frame sprite slug (pre-framed art)
	UPROPERTY() FName IconSlug;           // small icon fallback
	UPROPERTY() FName SfxSlug;            // resolve sound
	UPROPERTY() FName ProjectileSlug;     // spline projectile sprite (optional)
	UPROPERTY() FName ParticleSlug;       // impact particle sprite (optional)
};

// ---------------------------------------------------------------- Species

USTRUCT()
struct FBfkSpeciesDef
{
	GENERATED_BODY()

	UPROPERTY() FName Slug;
	UPROPERTY() FString Display;
	UPROPERTY() FString Desc;
	UPROPERTY() EBfkElement Element = EBfkElement::Neutral;
	UPROPERTY() EBfkArchetype Archetype = EBfkArchetype::Bruiser;
	UPROPERTY() int32 Quality = 1;        // 1 common 2 elite 3 legendary
	UPROPERTY() int32 MaxHp = 30;
	UPROPERTY() int32 Power = 0;          // generic damage bonus scalar
	UPROPERTY() TArray<FName> SignatureCards;
	UPROPERTY() FName SpriteSlug;
	UPROPERTY() bool bBeast = true;       // capturable & breedable
	UPROPERTY() bool bHumanoid = false;   // hire-only heroes
	UPROPERTY() bool bEnemyOnly = false;
	UPROPERTY() bool bBoss = false;
	UPROPERTY() bool bMinion = false;     // summonable trash
};

// ---------------------------------------------------------------- Gear

UENUM()
enum class EBfkRelicHook : uint8
{
	OnBattleStart, OnTurnStart, OnTurnEnd, OnCardPlayed, OnAttack,
	OnDamaged, OnKill, OnHpBelowHalf, OnShove, Passive
};

USTRUCT()
struct FBfkRelicDef
{
	GENERATED_BODY()

	UPROPERTY() FName Slug;
	UPROPERTY() FString Display;
	UPROPERTY() FString Desc;             // includes the cost/downside wording
	UPROPERTY() EBfkRarity Rarity = EBfkRarity::Uncommon;
	UPROPERTY() FName SpriteSlug;
	// Simple data-driven hooks; complex relics are special-cased by slug in code.
	UPROPERTY() EBfkRelicHook Hook = EBfkRelicHook::Passive;
	UPROPERTY() TArray<FBfkEffect> HookEffects;      // applied to the holder
	UPROPERTY() int32 HpMod = 0;
	UPROPERTY() int32 PowerMod = 0;
	UPROPERTY() int32 EnergyMod = 0;                 // squad-wide energy delta
	UPROPERTY() FString UnlockHint;                  // how it's earned (milestones)
};

USTRUCT()
struct FBfkWeaponDef
{
	GENERATED_BODY()

	UPROPERTY() FName Slug;
	UPROPERTY() FString Display;
	UPROPERTY() FString Desc;
	UPROPERTY() FString WClass;           // sword/axe/bow/staff/...
	UPROPERTY() EBfkElement Element = EBfkElement::Neutral;
	UPROPERTY() EBfkRarity Rarity = EBfkRarity::Common;
	UPROPERTY() FName SpriteSlug;
	UPROPERTY() TArray<FName> GrantedCards;          // 1-3 card slugs added to deck
	UPROPERTY() int32 HpMod = 0;
	UPROPERTY() int32 PowerMod = 0;
};

// ---------------------------------------------------------------- Run map

UENUM()
enum class EBfkNode : uint8
{
	Battle, Elite, Event, Shop, Forge, BreedingDen, Rest, Boss
};

USTRUCT()
struct FBfkMapNode
{
	GENERATED_BODY()

	UPROPERTY() int32 Id = 0;
	UPROPERTY() EBfkNode Type = EBfkNode::Battle;
	UPROPERTY() int32 Floor = 0;
	UPROPERTY() float X = 0.f;            // 0..1 horizontal placement
	UPROPERTY() TArray<int32> Next;       // node ids on next floor
	UPROPERTY() bool bVisited = false;
	UPROPERTY() int32 Seed = 0;           // encounter/event determinism
};

// ---------------------------------------------------------------- Profile

USTRUCT()
struct FBfkOwnedBeast
{
	GENERATED_BODY()

	UPROPERTY() FGuid Id;
	UPROPERTY() FName Species;
	UPROPERTY() FString Nickname;          // empty = species display
	UPROPERTY() int32 BonusHp = 0;         // breeding inheritance
	UPROPERTY() int32 BonusPower = 0;
	UPROPERTY() FName InheritedCard;       // extra signature card (bred)
	UPROPERTY() int32 Generation = 0;      // 0 = wild-caught
	UPROPERTY() FString ParentNote;
	UPROPERTY() int32 Victories = 0;
	UPROPERTY() int32 Level = 0;           // Forgedust levels (stat bumps live in BonusHp/BonusPower)
};

USTRUCT()
struct FBfkEgg
{
	GENERATED_BODY()

	UPROPERTY() FGuid Id;
	UPROPERTY() FName ResultSpecies;
	UPROPERTY() int32 BonusHp = 0;
	UPROPERTY() int32 BonusPower = 0;
	UPROPERTY() FName InheritedCard;
	UPROPERTY() int32 Generation = 1;
	UPROPERTY() int32 BattlesToHatch = 3;
	UPROPERTY() FString ParentNote;
};

// ---------------------------------------------------------------- Board

namespace Bfk
{
	// 5v5 hex battlefield: 5 lanes (rows) x 9 columns, "odd-r" offset hexes
	// (odd rows are shifted half a hex right). Ally half cols 0-3, enemy half
	// 5-8, col 4 is the contested middle.
	constexpr int32 BoardRows = 5;
	constexpr int32 BoardCols = 9;
	constexpr int32 AllyBackCol = 1, AllyFrontCol = 2;
	constexpr int32 EnemyFrontCol = 6, EnemyBackCol = 7;
	constexpr int32 SquadSize = 5;
}

USTRUCT()
struct FBfkCell
{
	GENERATED_BODY()

	UPROPERTY() int32 Row = 0;   // 0..BoardRows-1 (lane)
	UPROPERTY() int32 Col = 0;   // 0..BoardCols-1

	FBfkCell() {}
	FBfkCell(int32 R, int32 C) : Row(R), Col(C) {}
	bool operator==(const FBfkCell& O) const { return Row == O.Row && Col == O.Col; }
	bool IsValid() const { return Row >= 0 && Row < Bfk::BoardRows && Col >= 0 && Col < Bfk::BoardCols; }
	bool IsAllySide() const { return Col <= 3; }
	bool IsEnemySide() const { return Col >= 5; }
};

FORCEINLINE uint32 GetTypeHash(const FBfkCell& C) { return C.Row * 32 + C.Col; }

namespace Bfk
{
	// true hex distance on the odd-r offset grid (via cube coords)
	FORCEINLINE int32 HexDist(const FBfkCell& A, const FBfkCell& B)
	{
		const int32 Aq = A.Col - (A.Row - (A.Row & 1)) / 2;
		const int32 Bq = B.Col - (B.Row - (B.Row & 1)) / 2;
		const int32 Dq = Aq - Bq, Dr = A.Row - B.Row;
		return (FMath::Abs(Dq) + FMath::Abs(Dr) + FMath::Abs(Dq + Dr)) / 2;
	}

	// the (up to) 6 neighbors of a hex, odd-r offset
	FORCEINLINE TArray<FBfkCell, TInlineAllocator<6>> HexNeighbors(const FBfkCell& C)
	{
		const bool bOdd = (C.Row & 1) != 0;
		const int32 L = bOdd ? 0 : -1;   // row-diagonal col offsets
		const int32 R = bOdd ? 1 : 0;
		const FBfkCell N[6] = {
			{C.Row, C.Col - 1}, {C.Row, C.Col + 1},
			{C.Row - 1, C.Col + L}, {C.Row - 1, C.Col + R},
			{C.Row + 1, C.Col + L}, {C.Row + 1, C.Col + R}};
		TArray<FBfkCell, TInlineAllocator<6>> Out;
		for (const FBfkCell& X : N) if (X.IsValid()) Out.Add(X);
		return Out;
	}

	// attack range / move speed by archetype
	FORCEINLINE int32 ArchetypeRange(EBfkArchetype A)
	{
		switch (A)
		{
		case EBfkArchetype::Caster:    return 4;
		case EBfkArchetype::Support:   return 3;
		case EBfkArchetype::Striker:   return 2;
		case EBfkArchetype::Trickster: return 2;
		default:                       return 1;   // Bruiser / Tank
		}
	}
	FORCEINLINE int32 ArchetypeMove(EBfkArchetype A)
	{
		switch (A)
		{
		case EBfkArchetype::Striker:   return 3;
		case EBfkArchetype::Trickster: return 3;
		case EBfkArchetype::Tank:      return 1;
		default:                       return 2;
		}
	}
}

namespace Bfk
{
	FString ElementName(EBfkElement E);
	FLinearColor ElementColor(EBfkElement E);
	FString StatusName(EBfkStatus S);
	FString StatusDesc(EBfkStatus S);
	FString HazardName(EBfkHazard H);
	FString HazardDesc(EBfkHazard H);
	FString WeatherName(EBfkWeather W);
	FString WeatherDesc(EBfkWeather W);
	FString ArchetypeName(EBfkArchetype A);
	FString RarityName(EBfkRarity R);
	FLinearColor RarityColor(EBfkRarity R);
}
