#include "BfkBattle.h"
#include "Core/BfkContent.h"

namespace
{
	constexpr int32 kCollisionDamage = 3;
	constexpr int32 kKegDamage = 8;
	constexpr int32 kTidePeriod = 3;
	constexpr int32 kCapsizePeriod = 3;
}

// ============================================================== setup

void UBfkBattle::Init(const FBfkBattleConfig& InConfig)
{
	FBfkContent::EnsureInit();
	Config = InConfig;
	Rng.Initialize(Config.Seed);

	BuildSide(Config.Allies, false);
	BuildSide(Config.Enemies, true);

	BuildDeck(0);
	if (Config.bPvp)
	{
		BuildDeck(1);
	}

	// Soul snare token for capture runs
	if (Config.bAllowCapture && !Config.bPvp)
	{
		FBfkCardInstance Snare;
		Snare.InstanceId = NextCardId++;
		Snare.CardSlug = TEXT("soul-snare");
		Snare.OwnerUnitId = -1;
		Snare.bToken = true;
		DrawPile[0].Add(Snare);
	}

	ShuffleDraw(0);
	if (Config.bPvp) ShuffleDraw(1);

	Turn = 0;
	ActiveSide = 0;
	if (!Config.bPvp) PickIntents();
	StartTurn(0);
}

void UBfkBattle::BuildSide(const TArray<FBfkFighterSpec>& Specs, bool bEnemy)
{
	// Default formation per archetype: tanks/bruisers front, casters/support back.
	for (int32 i = 0; i < Specs.Num() && i < Bfk::SquadSize; ++i)
	{
		const FBfkFighterSpec& Spec = Specs[i];
		const FBfkSpeciesDef* Sp = FBfkContent::Species(Spec.Species);
		if (!Sp) continue;

		FBfkUnitState U;
		U.Id = NextUnitId++;
		U.Species = Spec.Species;
		U.Display = Spec.Display.IsEmpty() ? Sp->Display : Spec.Display;
		U.bEnemySide = bEnemy;
		U.MaxHp = Sp->MaxHp + Spec.BonusHp;
		U.Power = Sp->Power + Spec.BonusPower;
		U.Weapon = Spec.Weapon;
		U.Relic = Spec.Relic;
		U.bBoss = Sp->bBoss;
		U.bBeast = Sp->bBeast;
		U.VaultId = Spec.VaultId;

		if (const FBfkWeaponDef* W = FBfkContent::Weapon(Spec.Weapon))
		{
			U.MaxHp += W->HpMod;
			U.Power += W->PowerMod;
		}
		if (const FBfkRelicDef* R = FBfkContent::Relic(Spec.Relic))
		{
			U.MaxHp += R->HpMod;
			U.Power += R->PowerMod;
			MaxEnergy[bEnemy ? 1 : 0] += R->EnergyMod;
		}
		U.MaxHp = FMath::Max(1, U.MaxHp);
		U.Hp = U.MaxHp;

		U.Range = Bfk::ArchetypeRange(Sp->Archetype);
		U.Move = Bfk::ArchetypeMove(Sp->Archetype);
		if (Sp->bBoss) { U.Range = FMath::Max(U.Range, 4); U.Move = 1; }
		// reach weapons extend range
		if (const FBfkWeaponDef* W = FBfkContent::Weapon(Spec.Weapon))
		{
			if (W->WClass == TEXT("bow") || W->WClass == TEXT("crossbow")
				|| W->WClass == TEXT("staff") || W->WClass == TEXT("wand"))
			{
				U.Range += 1;
			}
		}

		const bool bFronty = Sp->Archetype == EBfkArchetype::Bruiser || Sp->Archetype == EBfkArchetype::Tank
			|| Sp->Archetype == EBfkArchetype::Striker;
		const int32 FrontCol = bEnemy ? Bfk::EnemyFrontCol : Bfk::AllyFrontCol;
		const int32 BackCol = bEnemy ? Bfk::EnemyBackCol : Bfk::AllyBackCol;
		U.Cell = FBfkCell(i, bFronty ? FrontCol : BackCol);
		// Bosses take the middle lane front and are the only unit placed there.
		if (Sp->bBoss) U.Cell = FBfkCell(Bfk::BoardRows / 2, FrontCol);
		while (UnitAt(U.Cell)) // resolve collisions in placement
		{
			U.Cell = FindFreeCell(bEnemy, U.Cell.Row);
		}
		Units.Add(U);
	}
}

void UBfkBattle::BuildDeck(int32 Side)
{
	const TArray<FBfkFighterSpec>& Specs = (Side == 0) ? Config.Allies : Config.Enemies;
	for (const FBfkFighterSpec& Spec : Specs)
	{
		// find matching built unit
		int32 OwnerId = -1;
		for (const FBfkUnitState& U : Units)
		{
			if (U.Species == Spec.Species && (U.bEnemySide == (Side == 1)))
			{
				OwnerId = U.Id; // first match is fine (dup species share cards)
			}
		}
		for (FName CardSlug : FBfkContent::DeckFor(Spec))
		{
			FBfkCardInstance CI;
			CI.InstanceId = NextCardId++;
			CI.CardSlug = CardSlug;
			CI.OwnerUnitId = OwnerId;
			DrawPile[Side].Add(CI);
		}
	}
}

void UBfkBattle::ShuffleDraw(int32 Side)
{
	for (int32 i = DrawPile[Side].Num() - 1; i > 0; --i)
	{
		DrawPile[Side].Swap(i, Rng.RandRange(0, i));
	}
	Emit(EBfkEvt::DeckShuffled, -1, -1, 0, Side);
}

// ============================================================== queries

const FBfkUnitState* UBfkBattle::FindUnit(int32 Id) const
{
	for (const FBfkUnitState& U : Units) if (U.Id == Id) return &U;
	return nullptr;
}

FBfkUnitState* UBfkBattle::MutFind(int32 Id)
{
	for (FBfkUnitState& U : Units) if (U.Id == Id) return &U;
	return nullptr;
}

const FBfkUnitState* UBfkBattle::UnitAt(const FBfkCell& Cell) const
{
	for (const FBfkUnitState& U : Units) if (U.bAlive && U.Cell == Cell) return &U;
	return nullptr;
}

FBfkUnitState* UBfkBattle::MutUnitAt(const FBfkCell& Cell)
{
	for (FBfkUnitState& U : Units) if (U.bAlive && U.Cell == Cell) return &U;
	return nullptr;
}

EBfkHazard UBfkBattle::HazardAt(const FBfkCell& Cell) const
{
	const EBfkHazard* H = Hazards.Find(Cell);
	return H ? *H : EBfkHazard::None;
}

const FBfkCardInstance* UBfkBattle::FindCard(int32 InstanceId) const
{
	for (int32 S = 0; S < 2; ++S)
	{
		for (const FBfkCardInstance& C : Hand[S]) if (C.InstanceId == InstanceId) return &C;
		for (const FBfkCardInstance& C : DrawPile[S]) if (C.InstanceId == InstanceId) return &C;
		for (const FBfkCardInstance& C : Discard[S]) if (C.InstanceId == InstanceId) return &C;
	}
	return nullptr;
}

bool UBfkBattle::IsCardSevered(const FBfkCardInstance& CI) const
{
	if (CI.OwnerUnitId < 0) return false;
	const FBfkUnitState* Owner = FindUnit(CI.OwnerUnitId);
	return Owner && !Owner->bAlive;
}

bool UBfkBattle::GetIntent(int32 UnitId, FName& OutCard, int32& OutTargetCode, bool& bOutAdvance) const
{
	if (const FIntent* I = Intents.Find(UnitId))
	{
		OutCard = I->Card;
		OutTargetCode = I->TargetCode;
		bOutAdvance = I->bAdvance;
		return true;
	}
	return false;
}

const FBfkUnitState* UBfkBattle::NearestFoe(const FBfkUnitState& U) const
{
	const FBfkUnitState* Best = nullptr;
	int32 BestD = INT32_MAX;
	for (const FBfkUnitState& T : Units)
	{
		if (!T.bAlive || T.bEnemySide == U.bEnemySide) continue;
		const int32 D = Bfk::HexDist(U.Cell, T.Cell);
		if (D < BestD) { BestD = D; Best = &T; }
	}
	return Best;
}

// ============================================================== movement

bool UBfkBattle::CanUnitMove(int32 UnitId) const
{
	if (bOver) return false;
	const FBfkUnitState* U = FindUnit(UnitId);
	return U && U->bAlive && !U->bMovedThisTurn && U->Move > 0
		&& U->bEnemySide == (ActiveSide == 1);
}

TArray<int32> UBfkBattle::GetMoveCells(int32 UnitId) const
{
	TArray<int32> Out;
	if (!CanUnitMove(UnitId)) return Out;
	const FBfkUnitState* U = FindUnit(UnitId);
	for (int32 R = 0; R < Bfk::BoardRows; ++R)
	{
		for (int32 C = 0; C < Bfk::BoardCols; ++C)
		{
			const FBfkCell Cell(R, C);
			if (UnitAt(Cell)) continue;
			if (Bfk::HexDist(U->Cell, Cell) <= U->Move) Out.Add(CellCode(Cell));
		}
	}
	return Out;
}

bool UBfkBattle::MoveCommand(int32 UnitId, int32 InCellCode)
{
	if (!GetMoveCells(UnitId).Contains(InCellCode)) return false;
	FBfkUnitState* U = MutFind(UnitId);
	U->bMovedThisTurn = true;
	MoveUnit(*U, CodeCell(InCellCode), false);
	CheckEnd();
	return true;
}

const FBfkCardDef* UBfkBattle::Def(FName Slug) const
{
	return FBfkContent::Card(Slug);
}

int32 UBfkBattle::CountAlive(bool bEnemy) const
{
	// summoned reinforcements scatter when the original roster falls
	int32 Originals = 0, All = 0;
	for (const FBfkUnitState& U : Units)
	{
		if (!U.bAlive || U.bEnemySide != bEnemy) continue;
		++All;
		if (U.bOriginal) ++Originals;
	}
	return (Originals > 0) ? All : 0;
}

TArray<int32> UBfkBattle::SideUnitIds(bool bEnemy, bool bAliveOnly) const
{
	TArray<int32> Out;
	for (const FBfkUnitState& U : Units)
	{
		if (U.bEnemySide == bEnemy && (!bAliveOnly || U.bAlive)) Out.Add(U.Id);
	}
	return Out;
}

FBfkUnitState* UBfkBattle::FirstInLane(int32 Row, bool bEnemySide, bool bFromAllyView)
{
	// Scan the row toward the defender's back: the first unit of that side hit.
	if (bEnemySide)
	{
		for (int32 C = 0; C < Bfk::BoardCols; ++C)
			if (FBfkUnitState* U = MutUnitAt(FBfkCell(Row, C)))
				if (U->bEnemySide) return U;
	}
	else
	{
		for (int32 C = Bfk::BoardCols - 1; C >= 0; --C)
			if (FBfkUnitState* U = MutUnitAt(FBfkCell(Row, C)))
				if (!U->bEnemySide) return U;
	}
	return nullptr;
}

FBfkCell UBfkBattle::FindFreeCell(bool bEnemySide, int32 PreferRow)
{
	const TArray<int32> Cols = bEnemySide
		? TArray<int32>{Bfk::EnemyFrontCol, Bfk::EnemyBackCol, Bfk::EnemyBackCol + 1, 5}
		: TArray<int32>{Bfk::AllyFrontCol, Bfk::AllyBackCol, 0, 3};
	TArray<int32> Rows = {PreferRow};
	for (int32 D = 1; D < Bfk::BoardRows; ++D)
	{
		if (PreferRow + D < Bfk::BoardRows) Rows.Add(PreferRow + D);
		if (PreferRow - D >= 0) Rows.Add(PreferRow - D);
	}
	for (int32 R : Rows)
		for (int32 C : Cols)
			if (!UnitAt(FBfkCell(R, C))) return FBfkCell(R, C);
	return FBfkCell(-1, -1);
}

// ============================================================== events

void UBfkBattle::Emit(EBfkEvt Type, int32 Unit, int32 Unit2, int32 A, int32 B, int32 C, FName Slug, const FString& Str)
{
	FBfkBattleEvent E;
	E.Type = Type; E.Unit = Unit; E.Unit2 = Unit2; E.A = A; E.B = B; E.C = C; E.Slug = Slug; E.Str = Str;
	Events.Add(E);
}

// ============================================================== turn flow

void UBfkBattle::StartTurn(int32 Side)
{
	ActiveSide = Side;
	if (Side == 0) ++Turn;
	Emit(EBfkEvt::TurnBegan, -1, -1, Turn, Side);

	// banked energy from last turn carries over (capped at 3)
	Energy[Side] = MaxEnergy[Side] + CarryEnergy[Side];
	CarryEnergy[Side] = 0;

	// fresh move allowance
	for (FBfkUnitState& U : Units)
	{
		if (U.bEnemySide == (Side == 1)) U.bMovedThisTurn = false;
	}

	// Chill: 3+ stacks on any of your units freezes 1 energy (once per unit).
	for (const FBfkUnitState& U : Units)
	{
		if (U.bAlive && U.bEnemySide == (Side == 1) && U.Status(EBfkStatus::Chill) >= 3)
		{
			Energy[Side] = FMath::Max(0, Energy[Side] - 1);
		}
	}
	Emit(EBfkEvt::EnergyChanged, -1, -1, Energy[Side], Side);

	ApplyWeatherTurnStart(Side);
	TickStatusesTurnStart(Side);
	if (Side == 1 && !Config.bPvp) BossTurnStart();
	RelicHooksSide(EBfkRelicHook::OnTurnStart, Side);

	// hazards under each unit
	for (FBfkUnitState& U : Units)
	{
		if (U.bAlive && U.bEnemySide == (Side == 1)) TickHazardsFor(U);
	}

	if (Side == 0 || Config.bPvp)
	{
		// top up to a hand of 5 (kept cards count); always at least 1
		DrawCards(Side, FMath::Clamp(5 - Hand[Side].Num(), 1, 5));
	}
	CheckEnd();
}

void UBfkBattle::DrawCards(int32 Side, int32 N)
{
	for (int32 i = 0; i < N; ++i)
	{
		if (DrawPile[Side].Num() == 0)
		{
			if (Discard[Side].Num() == 0) break;
			DrawPile[Side] = Discard[Side];
			Discard[Side].Reset();
			ShuffleDraw(Side);
		}
		FBfkCardInstance CI = DrawPile[Side].Pop();
		Hand[Side].Add(CI);
		Emit(EBfkEvt::CardDrawn, -1, -1, CI.InstanceId, Side, 0, CI.CardSlug);
	}
}

void UBfkBattle::EndTurn(bool bKeepHand)
{
	if (bOver) return;
	const int32 Side = ActiveSide;

	TickStatusesTurnEnd(Side);
	RelicHooksSide(EBfkRelicHook::OnTurnEnd, Side);

	// bank unspent energy (capped at 3)
	CarryEnergy[Side] = FMath::Clamp(Energy[Side], 0, 3);

	// Discard hand (player's choice; Retain cards always stay)
	if ((Side == 0 || Config.bPvp) && !bKeepHand)
	{
		for (int32 i = Hand[Side].Num() - 1; i >= 0; --i)
		{
			const FBfkCardDef* D = Def(Hand[Side][i].CardSlug);
			const bool bRetain = D && D->Effects.ContainsByPredicate([](const FBfkEffect& E){ return E.Op == EBfkOp::Retain; });
			if (!bRetain)
			{
				Emit(EBfkEvt::CardDiscarded, -1, -1, Hand[Side][i].InstanceId, Side);
				Discard[Side].Add(Hand[Side][i]);
				Hand[Side].RemoveAt(i);
			}
		}
	}

	if (bOver) return;

	if (Config.bPvp)
	{
		StartTurn(1 - Side);
	}
	else if (Side == 0)
	{
		ResolveEnemyTurn();
	}
}

void UBfkBattle::ResolveEnemyTurn()
{
	StartTurn(1);
	if (bOver) return;

	// Each living enemy fires its telegraphed intent, top lane first.
	TArray<int32> Order = SideUnitIds(true);
	Order.Sort([this](int32 A, int32 B)
	{
		const FBfkUnitState* UA = FindUnit(A); const FBfkUnitState* UB = FindUnit(B);
		return UA->Cell.Row < UB->Cell.Row;
	});
	for (int32 Id : Order)
	{
		if (bOver) return;
		FBfkUnitState* E = MutFind(Id);
		if (!E || !E->bAlive) continue;
		if (const FIntent* I = Intents.Find(Id))
		{
			EnemyPlayIntent(*E, *I);
		}
	}

	TickStatusesTurnEnd(1);
	if (bOver) return;

	PickIntents();
	StartTurn(0);
}

void UBfkBattle::EnemyPlayIntent(FBfkUnitState& Enemy, const FIntent& Intent)
{
	if (Intent.bAdvance)
	{
		EnemyAdvance(Enemy);
		return;
	}
	const FBfkCardDef* D = Def(Intent.Card);
	if (!D) return;

	// If the intent had a unit target that moved lanes, the attack tracks the
	// CELL, not the unit — positioning counterplay.
	Emit(EBfkEvt::CardPlayed, Enemy.Id, -1, -1, 1, 0, Intent.Card);
	FBfkCardInstance Fake;
	Fake.InstanceId = -1;
	Fake.CardSlug = Intent.Card;
	Fake.OwnerUnitId = Enemy.Id;
	ResolveCard(Fake, *D, Intent.TargetCode, Enemy);
	CheckEnd();
}

void UBfkBattle::PickIntents()
{
	Intents.Reset();
	for (const FBfkUnitState& U : Units)
	{
		if (!U.bAlive || !U.bEnemySide) continue;
		const FBfkSpeciesDef* Sp = FBfkContent::Species(U.Species);
		if (!Sp || Sp->SignatureCards.Num() == 0) continue;

		FIntent I;
		I.Card = Sp->SignatureCards[Rng.RandRange(0, Sp->SignatureCards.Num() - 1)];
		const FBfkCardDef* D = Def(I.Card);
		if (!D) continue;

		// choose a target cell now (telegraph)
		switch (D->Target)
		{
		case EBfkTarget::Enemy:
		case EBfkTarget::EnemyFront:
		{
			// From the enemy's perspective, "enemies" are the player's units —
			// but only those within this unit's reach. Out of range = advance.
			const int32 Reach = D->bMeleeOnly ? 1 : U.Range;
			TArray<int32> Choices;
			for (const FBfkUnitState& T : Units)
			{
				if (!T.bAlive || T.bEnemySide) continue;
				if (T.Status(EBfkStatus::Stealth) > 0) continue;
				if (Bfk::HexDist(U.Cell, T.Cell) > Reach) continue;
				Choices.Add(T.Id);
			}
			if (Choices.Num() == 0)
			{
				I.Card = NAME_None;
				I.bAdvance = true;   // close the distance instead
				I.TargetCode = -1;
				break;
			}
			const FBfkUnitState* T = FindUnit(Choices[Rng.RandRange(0, Choices.Num() - 1)]);
			I.TargetCode = CellCode(T->Cell);
			break;
		}
		case EBfkTarget::Lane:
			I.TargetCode = CellCode(FBfkCell(U.Cell.Row, 0)); // its own lane
			break;
		case EBfkTarget::Ally: // enemy buffing its own side: pick weakest ally-of-enemy
		{
			int32 Best = -1, BestHp = INT32_MAX;
			for (const FBfkUnitState& T : Units)
			{
				if (T.bAlive && T.bEnemySide && T.Hp < BestHp) { Best = T.Id; BestHp = T.Hp; }
			}
			I.TargetCode = Best >= 0 ? CellCode(FindUnit(Best)->Cell) : -1;
			break;
		}
		case EBfkTarget::Cell:
		case EBfkTarget::AllySlot:
		{
			// hazard drop: prefer a cell under a player unit
			TArray<const FBfkUnitState*> Ps;
			for (const FBfkUnitState& T : Units) if (T.bAlive && !T.bEnemySide) Ps.Add(&T);
			if (Ps.Num() > 0) I.TargetCode = CellCode(Ps[Rng.RandRange(0, Ps.Num() - 1)]->Cell);
			break;
		}
		default:
			I.TargetCode = -1;
			break;
		}
		Intents.Add(U.Id, I);
		Emit(EBfkEvt::IntentUpdated, U.Id, -1, I.TargetCode, 0, 0, I.Card);
	}
}

void UBfkBattle::EnemyAdvance(FBfkUnitState& Enemy)
{
	const FBfkUnitState* Target = NearestFoe(Enemy);
	if (!Target) return;
	// greedy hex-walk: each step take the neighbor closest to the target
	for (int32 Step = 0; Step < Enemy.Move; ++Step)
	{
		if (Bfk::HexDist(Enemy.Cell, Target->Cell) <= FMath::Max(1, Enemy.Range)) break;
		FBfkCell Best = Enemy.Cell;
		int32 BestD = Bfk::HexDist(Enemy.Cell, Target->Cell);
		for (const FBfkCell& N : Bfk::HexNeighbors(Enemy.Cell))
		{
			if (UnitAt(N)) continue;
			const int32 D = Bfk::HexDist(N, Target->Cell);
			if (D < BestD) { BestD = D; Best = N; }
		}
		if (Best == Enemy.Cell) break;   // boxed in
		MoveUnit(Enemy, Best, false);
	}
}

// ============================================================== statuses / weather / hazards

void UBfkBattle::TickStatusesTurnStart(int32 Side)
{
	for (FBfkUnitState& U : Units)
	{
		if (!U.bAlive || U.bEnemySide != (Side == 1)) continue;

		// Block decays at turn start (Ward persists as its own pool)
		U.Block = 0;

		if (int32 Burn = U.Status(EBfkStatus::Burn))
		{
			DealDamage(nullptr, U, Burn, false);
			ApplyStatus(U, EBfkStatus::Burn, -1);
		}
	}
}

void UBfkBattle::TickStatusesTurnEnd(int32 Side)
{
	for (FBfkUnitState& U : Units)
	{
		if (!U.bAlive || U.bEnemySide != (Side == 1)) continue;

		if (int32 Poison = U.Status(EBfkStatus::Poison))
		{
			DealDamage(nullptr, U, Poison, false);
			ApplyStatus(U, EBfkStatus::Poison, -1); // fades (was festering — too oppressive)
		}
		for (EBfkStatus S : {EBfkStatus::Chill, EBfkStatus::Curse, EBfkStatus::Rust, EBfkStatus::Rally, EBfkStatus::Stealth})
		{
			if (U.Status(S) > 0) ApplyStatus(U, S, -1);
		}
	}
}

void UBfkBattle::TickHazardsFor(FBfkUnitState& U)
{
	const EBfkHazard H = HazardAt(U.Cell);
	if (H == EBfkHazard::None) return;
	Emit(EBfkEvt::HazardTriggered, U.Id, -1, CellCode(U.Cell), (int32)H);
	switch (H)
	{
	case EBfkHazard::Coals:     ApplyStatus(U, EBfkStatus::Burn, 2); break;
	case EBfkHazard::Hoarfrost: ApplyStatus(U, EBfkStatus::Chill, 2); break;
	case EBfkHazard::VoidRift:  ApplyStatus(U, EBfkStatus::Curse, 1); break;
	case EBfkHazard::Spores:    ApplyStatus(U, EBfkStatus::Poison, 2); break;
	default: break;
	}
}

void UBfkBattle::ApplyWeatherTurnStart(int32 Side)
{
	Emit(EBfkEvt::WeatherPulse, -1, -1, (int32)Config.Weather);
	if (Config.Weather == EBfkWeather::Snow && (Turn % 2 == 0))
	{
		// exposed back column gathers chill
		const int32 BackCol = (Side == 1) ? Bfk::BoardCols - 1 : 0;
		for (FBfkUnitState& U : Units)
		{
			if (U.bAlive && U.bEnemySide == (Side == 1) && U.Cell.Col == BackCol)
			{
				ApplyStatus(U, EBfkStatus::Chill, 1);
			}
		}
	}
	if (Config.Weather == EBfkWeather::Rain && Side == 0 && (Turn % 3 == 1))
	{
		// puddles pool on random empty cells (bigger field = a few more)
		for (int32 i = 0; i < 3; ++i)
		{
			FBfkCell C(Rng.RandRange(0, Bfk::BoardRows - 1), Rng.RandRange(0, Bfk::BoardCols - 1));
			if (HazardAt(C) == EBfkHazard::None) PlaceHazard(C, EBfkHazard::Puddle);
		}
	}
}

// ============================================================== boss mechanics

void UBfkBattle::BossTurnStart()
{
	FBfkUnitState* Boss = nullptr;
	for (FBfkUnitState& U : Units) if (U.bAlive && U.bEnemySide && U.bBoss) { Boss = &U; break; }
	if (!Boss) return;

	const FString BossSlug = Boss->Species.ToString();

	if (BossSlug == TEXT("rot-shepherd"))
	{
		// spores creep across the board
		TArray<FBfkCell> SporeCells;
		for (const auto& KV : Hazards) if (KV.Value == EBfkHazard::Spores) SporeCells.Add(KV.Key);
		if (SporeCells.Num() == 0)
		{
			PlaceHazard(FBfkCell(Rng.RandRange(0, Bfk::BoardRows - 1), Rng.RandRange(0, 3)), EBfkHazard::Spores);
		}
		else
		{
			const FBfkCell Src = SporeCells[Rng.RandRange(0, SporeCells.Num() - 1)];
			const FBfkCell Dst(FMath::Clamp(Src.Row + Rng.RandRange(-1, 1), 0, Bfk::BoardRows - 1),
			                   FMath::Clamp(Src.Col + Rng.RandRange(-1, 1), 0, Bfk::BoardCols - 1));
			if (HazardAt(Dst) == EBfkHazard::None) PlaceHazard(Dst, EBfkHazard::Spores);
		}
		if (Turn % 3 == 0 && CountAlive(true) < 8)
		{
			SummonMinion(TEXT("min-sporeling"), true, FindFreeCell(true, Rng.RandRange(0, 2)));
			Emit(EBfkEvt::Speech, Boss->Id, -1, 0, 0, 0, NAME_None, TEXT("The wilds keep what they catch."));
		}
	}
	else if (BossSlug == TEXT("ghostwake"))
	{
		if (++CapsizeCounter >= kCapsizePeriod)
		{
			CapsizeCounter = 0;
			// capsize: mirror the player's half — front becomes back
			for (FBfkUnitState& U : Units)
			{
				if (U.bAlive && !U.bEnemySide && U.Cell.Col <= 3)
				{
					FBfkCell From = U.Cell;
					U.Cell.Col = 3 - U.Cell.Col;
					Emit(EBfkEvt::UnitMoved, U.Id, -1, CellCode(From), CellCode(U.Cell), 1);
				}
			}
			Emit(EBfkEvt::ColumnsCapsized);
			Emit(EBfkEvt::Speech, Boss->Id, -1, 0, 0, 0, NAME_None, TEXT("All hands... below."));
		}
		if (Turn % 3 == 2 && CountAlive(true) < 8)
		{
			SummonMinion(TEXT("min-deckhand"), true, FindFreeCell(true, Rng.RandRange(0, 2)));
		}
	}
	else if (BossSlug == TEXT("facet-queen"))
	{
		bFacetReflectArmed = true;
		Emit(EBfkEvt::Speech, Boss->Id, -1, 0, 0, 0, NAME_None, TEXT("Strike, and study your reflection."));
		if (Boss->Hp < Boss->MaxHp / 2 && Turn % 2 == 0)
		{
			FBfkCell C(Rng.RandRange(0, Bfk::BoardRows - 1), Rng.RandRange(0, 3));
			if (HazardAt(C) == EBfkHazard::None) PlaceHazard(C, EBfkHazard::CrystalShard);
		}
	}
	else if (BossSlug == TEXT("tidebound-king"))
	{
		++TideCounter;
		if (TideRow >= 0)
		{
			// flood the marked row
			Emit(EBfkEvt::TideFlood, -1, -1, TideRow);
			for (FBfkUnitState& U : Units)
			{
				if (U.bAlive && U.Cell.Row == TideRow && !U.bBoss)
				{
					DealDamage(nullptr, U, 6, false);
					if (U.bAlive && U.Cell.Row > 0)
					{
						FBfkCell Dest(U.Cell.Row - 1, U.Cell.Col);
						if (FBfkUnitState* Blocker = MutUnitAt(Dest))
						{
							Emit(EBfkEvt::Collision, U.Id, Blocker->Id, kCollisionDamage);
							DealDamage(nullptr, U, kCollisionDamage, false);
							DealDamage(nullptr, *Blocker, kCollisionDamage, false);
						}
						else
						{
							MoveUnit(U, Dest, true);
						}
					}
				}
			}
			for (int32 C = 0; C < Bfk::BoardCols; ++C) if (HazardAt(FBfkCell(TideRow, C)) == EBfkHazard::Floodmark) PlaceHazard(FBfkCell(TideRow, C), EBfkHazard::None);
			TideRow = -1;
		}
		else if (TideCounter % kTidePeriod == 0)
		{
			TideRow = Rng.RandRange(0, Bfk::BoardRows - 1);
			Emit(EBfkEvt::TideWarning, -1, -1, TideRow);
			for (int32 C = 0; C < Bfk::BoardCols; ++C) PlaceHazard(FBfkCell(TideRow, C), EBfkHazard::Floodmark);
			Emit(EBfkEvt::Speech, Boss->Id, -1, 0, 0, 0, NAME_None, TEXT("The sea remembers what I owe it."));
		}
	}
}

// ============================================================== relics

void UBfkBattle::RelicHooksSide(EBfkRelicHook Hook, int32 Side, int32 Magnitude)
{
	for (FBfkUnitState& U : Units)
	{
		if (U.bAlive && U.bEnemySide == (Side == 1) && !U.Relic.IsNone())
		{
			RelicHook(Hook, U, Magnitude);
		}
	}
}

void UBfkBattle::RelicHook(EBfkRelicHook Hook, FBfkUnitState& Holder, int32 Magnitude)
{
	const FBfkRelicDef* R = FBfkContent::Relic(Holder.Relic);
	if (!R || R->Hook != Hook) return;

	for (const FBfkEffect& Fx : R->HookEffects)
	{
		FBfkCardDef Dummy;
		ApplyEffect(Fx, Holder, CellCode(Holder.Cell), Dummy);
	}
}

// ============================================================== card play

bool UBfkBattle::CanPlayCard(int32 InstanceId, FString& WhyNot) const
{
	const int32 Side = ActiveSide;
	const FBfkCardInstance* CI = nullptr;
	for (const FBfkCardInstance& C : Hand[Side]) if (C.InstanceId == InstanceId) { CI = &C; break; }
	if (!CI) { WhyNot = TEXT("Not in hand."); return false; }
	if (bOver) { WhyNot = TEXT("Battle is over."); return false; }
	if (IsCardSevered(*CI)) { WhyNot = TEXT("Its owner has fallen — the bond is severed."); return false; }

	const FBfkCardDef* D = Def(CI->CardSlug);
	if (!D) { WhyNot = TEXT("Unknown card."); return false; }
	if (D->Cost > Energy[Side]) { WhyNot = TEXT("Not enough energy."); return false; }

	if (D->bMeleeOnly && CI->OwnerUnitId >= 0)
	{
		const FBfkUnitState* Owner = FindUnit(CI->OwnerUnitId);
		if (Owner && (D->Target == EBfkTarget::Enemy || D->Target == EBfkTarget::EnemyFront))
		{
			bool bFoeAdjacent = false;
			for (const FBfkUnitState& T : Units)
			{
				if (T.bAlive && T.bEnemySide != Owner->bEnemySide
					&& Bfk::HexDist(Owner->Cell, T.Cell) <= 1) { bFoeAdjacent = true; break; }
			}
			if (!bFoeAdjacent) { WhyNot = TEXT("Melee: no foe in reach — move closer."); return false; }
		}
	}
	return true;
}

TArray<int32> UBfkBattle::GetValidUnitTargets(int32 InstanceId) const
{
	TArray<int32> Out;
	const int32 Side = ActiveSide;
	const FBfkCardInstance* CI = nullptr;
	for (const FBfkCardInstance& C : Hand[Side]) if (C.InstanceId == InstanceId) { CI = &C; break; }
	if (!CI) return Out;
	const FBfkCardDef* D = Def(CI->CardSlug);
	if (!D) return Out;

	const bool bMyEnemies = (Side == 0); // side 0 targets bEnemySide units
	const FBfkUnitState* Owner = FindUnit(CI->OwnerUnitId);
	switch (D->Target)
	{
	case EBfkTarget::Enemy:
	case EBfkTarget::EnemyFront:
		for (const FBfkUnitState& U : Units)
		{
			if (!U.bAlive || U.bEnemySide != bMyEnemies) continue;
			if (U.Status(EBfkStatus::Stealth) > 0) continue;
			// range gate: melee cards & EnemyFront reach 1 hex, otherwise the
			// owner's Range. Token cards (snare) have no owner = unlimited.
			if (Owner)
			{
				const int32 Reach = (D->bMeleeOnly || D->Target == EBfkTarget::EnemyFront)
					? 1 : FMath::Max(1, Owner->Range);
				if (Bfk::HexDist(Owner->Cell, U.Cell) > Reach) continue;
			}
			// capture: beasts only, non-boss
			const bool bIsSnare = CI->CardSlug == TEXT("soul-snare");
			if (bIsSnare && (!U.bBeast || U.bBoss)) continue;
			Out.Add(U.Id);
		}
		break;
	case EBfkTarget::Ally:
		for (const FBfkUnitState& U : Units)
		{
			if (U.bAlive && U.bEnemySide != bMyEnemies) Out.Add(U.Id);
		}
		break;
	default: break;
	}
	return Out;
}

TArray<int32> UBfkBattle::GetValidCellTargets(int32 InstanceId) const
{
	TArray<int32> Out;
	const int32 Side = ActiveSide;
	const FBfkCardInstance* CI = nullptr;
	for (const FBfkCardInstance& C : Hand[Side]) if (C.InstanceId == InstanceId) { CI = &C; break; }
	if (!CI) return Out;
	const FBfkCardDef* D = Def(CI->CardSlug);
	if (!D) return Out;

	switch (D->Target)
	{
	case EBfkTarget::Lane:
		for (int32 R = 0; R < Bfk::BoardRows; ++R) Out.Add(R * 10);
		break;
	case EBfkTarget::Cell:
		for (int32 R = 0; R < Bfk::BoardRows; ++R)
			for (int32 C = 0; C < Bfk::BoardCols; ++C) Out.Add(R * 10 + C);
		break;
	case EBfkTarget::AllySlot:
	{
		const bool bEnemySlots = (Side == 1);
		for (int32 R = 0; R < Bfk::BoardRows; ++R)
			for (int32 C = bEnemySlots ? 5 : 0; C <= (bEnemySlots ? Bfk::BoardCols - 1 : 3); ++C)
				if (!UnitAt(FBfkCell(R, C))) Out.Add(R * 10 + C);
		break;
	}
	default: break;
	}
	return Out;
}

bool UBfkBattle::PlayCard(int32 InstanceId, int32 TargetCode)
{
	FString Why;
	if (!CanPlayCard(InstanceId, Why)) return false;

	const int32 Side = ActiveSide;
	int32 HandIdx = INDEX_NONE;
	for (int32 i = 0; i < Hand[Side].Num(); ++i) if (Hand[Side][i].InstanceId == InstanceId) { HandIdx = i; break; }
	const FBfkCardInstance CI = Hand[Side][HandIdx];
	const FBfkCardDef* D = Def(CI.CardSlug);

	// validate target
	if (D->Target == EBfkTarget::Enemy || D->Target == EBfkTarget::EnemyFront || D->Target == EBfkTarget::Ally)
	{
		if (!GetValidUnitTargets(InstanceId).Contains(TargetCode)) return false;
	}
	else if (D->Target == EBfkTarget::Lane || D->Target == EBfkTarget::Cell || D->Target == EBfkTarget::AllySlot)
	{
		if (!GetValidCellTargets(InstanceId).Contains(TargetCode)) return false;
	}

	Hand[Side].RemoveAt(HandIdx);
	Energy[Side] -= D->Cost;
	Emit(EBfkEvt::EnergyChanged, -1, -1, Energy[Side], Side);

	FBfkUnitState* Owner = MutFind(CI.OwnerUnitId);
	FBfkUnitState Fallback; // token cards (snare) act through a virtual owner
	if (!Owner)
	{
		Fallback.Id = -1;
		Fallback.bEnemySide = (Side == 1);
		Fallback.Cell = FBfkCell(Bfk::BoardRows / 2, Side == 1 ? Bfk::EnemyFrontCol : Bfk::AllyFrontCol);
		Owner = &Fallback;
	}

	Emit(EBfkEvt::CardPlayed, Owner->Id, -1, CI.InstanceId, Side, 0, CI.CardSlug);
	ResolveCard(CI, *D, TargetCode, *Owner);

	// exhaust or discard
	const bool bExhaust = D->Effects.ContainsByPredicate([](const FBfkEffect& E){ return E.Op == EBfkOp::ExhaustSelf; });
	if (bExhaust || CI.bToken)
	{
		Exhausted[Side].Add(CI);
		Emit(EBfkEvt::CardExhausted, -1, -1, CI.InstanceId, Side);
	}
	else
	{
		Discard[Side].Add(CI);
		Emit(EBfkEvt::CardDiscarded, -1, -1, CI.InstanceId, Side);
	}

	if (Owner->Id >= 0) RelicHook(EBfkRelicHook::OnCardPlayed, *Owner);
	CheckEnd();
	return true;
}

void UBfkBattle::ResolveCard(const FBfkCardInstance& CI, const FBfkCardDef& InDef, int32 TargetCode, FBfkUnitState& Owner)
{
	const TArray<FBfkEffect>& Effects = (CI.bUpgraded && InDef.UpgradedEffects.Num() > 0)
		? InDef.UpgradedEffects : InDef.Effects;

	const bool bIsAttackCard = InDef.Kind == EBfkCardKind::Attack;
	if (bIsAttackCard && Owner.Id >= 0)
	{
		RelicHook(EBfkRelicHook::OnAttack, Owner);
	}

	for (const FBfkEffect& Fx : Effects)
	{
		if (bOver) return;
		FBfkEffect Scaled = Fx;
		if (CI.bUpgraded && InDef.UpgradedEffects.Num() == 0 && Scaled.A > 0)
		{
			Scaled.A = FMath::CeilToInt(Scaled.A * 1.25f); // default upgrade
		}
		ApplyEffect(Scaled, Owner, TargetCode, InDef);
	}
}

int32 UBfkBattle::AttackDamage(const FBfkUnitState& Attacker, int32 Base) const
{
	int32 Dmg = Base + Attacker.Power;
	Dmg += Attacker.Status(EBfkStatus::Rally);
	if (Attacker.Status(EBfkStatus::Chill) > 0) Dmg -= 1;

	// weather
	const FBfkSpeciesDef* Sp = FBfkContent::Species(Attacker.Species);
	if (Sp)
	{
		switch (Config.Weather)
		{
		case EBfkWeather::Rain:
			if (Sp->Element == EBfkElement::Ember) Dmg -= 1;
			if (Sp->Element == EBfkElement::Storm) Dmg += 1;
			break;
		case EBfkWeather::Snow:
			if (Sp->Element == EBfkElement::Frost) Dmg += 1;
			break;
		case EBfkWeather::Ashfall:
			if (Sp->Element == EBfkElement::Shadow) Dmg += 1;
			break;
		default: break;
		}
	}
	if (HazardAt(Attacker.Cell) == EBfkHazard::CrystalShard) Dmg += 2;
	return FMath::Max(0, Dmg);
}

void UBfkBattle::ApplyEffect(const FBfkEffect& Fx, FBfkUnitState& Owner, int32 TargetCode, const FBfkCardDef& InDef)
{
	auto TargetUnit = [&]() -> FBfkUnitState*
	{
		// unit targets are passed as unit ids
		return MutFind(TargetCode);
	};
	auto TargetOrOwner = [&]() -> FBfkUnitState&
	{
		FBfkUnitState* T = MutFind(TargetCode);
		return T ? *T : Owner;
	};
	const bool bOwnerEnemySide = Owner.bEnemySide;

	switch (Fx.Op)
	{
	case EBfkOp::Damage:
	{
		FBfkUnitState* T = TargetUnit();
		if (!T)
		{
			// enemy intents target cells: resolve to occupant (dodged if empty)
			T = MutUnitAt(CodeCell(TargetCode));
		}
		if (T && T->bAlive)
		{
			Emit(EBfkEvt::UnitAttack, Owner.Id, T->Id, 0, 0, 0, InDef.Slug);
			if (!InDef.ProjectileSlug.IsNone())
			{
				Emit(EBfkEvt::ProjectileFly, Owner.Id, -1, CellCode(T->Cell), 0, 0, InDef.ProjectileSlug);
			}
			DealDamage(&Owner, *T, AttackDamage(Owner, Fx.A), true, InDef.Slug);
		}
		break;
	}
	case EBfkOp::DamageLane:
	{
		const int32 Row = TargetCode / 10;
		if (FBfkUnitState* T = FirstInLane(Row, !bOwnerEnemySide, !bOwnerEnemySide))
		{
			Emit(EBfkEvt::UnitAttack, Owner.Id, T->Id, 0, 0, 0, InDef.Slug);
			if (!InDef.ProjectileSlug.IsNone())
			{
				Emit(EBfkEvt::ProjectileFly, Owner.Id, -1, CellCode(T->Cell), 0, 0, InDef.ProjectileSlug);
			}
			DealDamage(&Owner, *T, AttackDamage(Owner, Fx.A), true, InDef.Slug);
		}
		break;
	}
	case EBfkOp::DamageRow:
	{
		const int32 Row = TargetCode / 10;
		for (FBfkUnitState& U : Units)
		{
			if (U.bAlive && U.bEnemySide != bOwnerEnemySide && U.Cell.Row == Row)
			{
				DealDamage(&Owner, U, AttackDamage(Owner, Fx.A), true, InDef.Slug);
			}
		}
		break;
	}
	case EBfkOp::DamageAll:
		for (FBfkUnitState& U : Units)
		{
			if (U.bAlive && U.bEnemySide != bOwnerEnemySide)
			{
				DealDamage(&Owner, U, AttackDamage(Owner, Fx.A), true, InDef.Slug);
			}
		}
		break;
	case EBfkOp::DamageSelf:
		DealDamage(nullptr, Owner, Fx.A, false);
		break;
	case EBfkOp::Block:
		GainBlock(TargetOrOwner(), Fx.A);
		break;
	case EBfkOp::BlockAll:
		for (FBfkUnitState& U : Units)
			if (U.bAlive && U.bEnemySide == bOwnerEnemySide) GainBlock(U, Fx.A);
		break;
	case EBfkOp::Heal:
		HealUnit(TargetOrOwner(), Fx.A);
		break;
	case EBfkOp::HealAll:
		for (FBfkUnitState& U : Units)
			if (U.bAlive && U.bEnemySide == bOwnerEnemySide) HealUnit(U, Fx.A);
		break;
	case EBfkOp::Status:
	{
		FBfkUnitState* T = TargetUnit();
		if (!T) T = MutUnitAt(CodeCell(TargetCode));
		if (!T) T = &Owner;
		if (T->bAlive)
		{
			ApplyStatus(*T, Fx.StatusA, Fx.A);
			// rain: shock chains across puddles
			if (Fx.StatusA == EBfkStatus::Shock && Config.Weather == EBfkWeather::Rain
				&& HazardAt(T->Cell) == EBfkHazard::Puddle)
			{
				for (FBfkUnitState& U : Units)
				{
					if (U.bAlive && U.Id != T->Id && HazardAt(U.Cell) == EBfkHazard::Puddle)
					{
						ApplyStatus(U, EBfkStatus::Shock, Fx.A);
					}
				}
			}
		}
		break;
	}
	case EBfkOp::StatusAll:
		for (FBfkUnitState& U : Units)
			if (U.bAlive && U.bEnemySide != bOwnerEnemySide) ApplyStatus(U, Fx.StatusA, Fx.A);
		break;
	case EBfkOp::StatusSelfSide:
		for (FBfkUnitState& U : Units)
			if (U.bAlive && U.bEnemySide == bOwnerEnemySide) ApplyStatus(U, Fx.StatusA, Fx.A);
		break;
	case EBfkOp::Cleanse:
	{
		FBfkUnitState& T = TargetOrOwner();
		int32 Left = Fx.A;
		for (EBfkStatus S : {EBfkStatus::Burn, EBfkStatus::Poison, EBfkStatus::Chill, EBfkStatus::Curse, EBfkStatus::Rust, EBfkStatus::Shock})
		{
			while (Left > 0 && T.Status(S) > 0) { ApplyStatus(T, S, -1); --Left; }
		}
		break;
	}
	case EBfkOp::Push:
	case EBfkOp::Pull:
	{
		FBfkUnitState* T = TargetUnit();
		if (!T) T = MutUnitAt(CodeCell(TargetCode));
		if (T && T->bAlive)
		{
			// push = away from owner side, pull = toward
			int32 Dir = (T->bEnemySide ? +1 : -1);
			if (Fx.Op == EBfkOp::Pull) Dir = -Dir;
			ShoveUnit(Owner, *T, Dir, 0, Fx.A);
		}
		break;
	}
	case EBfkOp::SwapAlly:
	{
		FBfkUnitState* T = TargetUnit();
		if (T && T->bAlive && T->Id != Owner.Id && Owner.Id >= 0)
		{
			const FBfkCell A = Owner.Cell, B = T->Cell;
			Owner.Cell = B; T->Cell = A;
			Emit(EBfkEvt::UnitMoved, Owner.Id, -1, CellCode(A), CellCode(B), 0);
			Emit(EBfkEvt::UnitMoved, T->Id, -1, CellCode(B), CellCode(A), 0);
			TickHazardsFor(Owner);
			TickHazardsFor(*T);
		}
		break;
	}
	case EBfkOp::MoveSelf:
	{
		const FBfkCell Dest = CodeCell(TargetCode);
		const bool bOnOwnHalf = bOwnerEnemySide ? Dest.IsEnemySide() : Dest.IsAllySide();
		if (Owner.Id >= 0 && Dest.IsValid() && !UnitAt(Dest) && bOnOwnHalf)
		{
			const FBfkCell From = Owner.Cell;
			MoveUnit(Owner, Dest, false);
			(void)From;
		}
		break;
	}
	case EBfkOp::Hazard:
		PlaceHazard(CodeCell(TargetCode), Fx.HazardA);
		break;
	case EBfkOp::ClearHazard:
		PlaceHazard(CodeCell(TargetCode), EBfkHazard::None);
		break;
	case EBfkOp::Draw:
		DrawCards(bOwnerEnemySide ? 1 : 0, Fx.A);
		break;
	case EBfkOp::Energy:
	{
		const int32 Side = bOwnerEnemySide ? 1 : 0;
		Energy[Side] += Fx.A;
		Emit(EBfkEvt::EnergyChanged, -1, -1, Energy[Side], Side);
		break;
	}
	case EBfkOp::Capture:
	{
		FBfkUnitState* T = TargetUnit();
		if (T && T->bAlive && T->bBeast && !T->bBoss && SnareCharges > 0)
		{
			--SnareCharges;
			const float HpFrac = (float)T->Hp / (float)T->MaxHp;
			const int32 Chance = FMath::Clamp(FMath::RoundToInt((1.f - HpFrac) * 90.f) + 10, 10, 95);
			const bool bSuccess = Rng.RandRange(1, 100) <= Chance;
			Emit(EBfkEvt::CaptureResult, T->Id, -1, bSuccess ? 1 : 0, Chance);
			if (bSuccess)
			{
				CapturedThisBattle.Add(T->Species);
				T->bAlive = false;
				T->Hp = 0;
				Emit(EBfkEvt::UnitDied, T->Id);
				SeverCards(T->Id);
			}
		}
		break;
	}
	case EBfkOp::SummonMinion:
		SummonMinion(Fx.SlugParam, bOwnerEnemySide, CodeCell(TargetCode));
		break;
	case EBfkOp::ExhaustSelf:
	case EBfkOp::Retain:
		break; // markers
	default: break;
	}
}

// ============================================================== combat primitives

void UBfkBattle::DealDamage(FBfkUnitState* Attacker, FBfkUnitState& Victim, int32 Amount, bool bIsAttack, FName CardSlug)
{
	if (!Victim.bAlive || Amount <= 0) return;

	// Facet Queen: reflect the first attack that hits her each enemy turn cycle
	if (bIsAttack && Attacker && Victim.bBoss && Victim.Species == TEXT("facet-queen") && bFacetReflectArmed)
	{
		bFacetReflectArmed = false;
		Emit(EBfkEvt::Speech, Victim.Id, -1, 0, 0, 0, NAME_None, TEXT("Refraction."));
		FBfkUnitState* Back = Attacker;
		DealDamage(nullptr, *Back, FMath::Max(1, Amount / 2), false);
		if (!Back->bAlive) return;
	}

	int32 Remaining = Amount;

	// shock detonation
	if (bIsAttack && Victim.Status(EBfkStatus::Shock) > 0)
	{
		const int32 Shock = Victim.Status(EBfkStatus::Shock);
		Remaining += Shock;
		Victim.Statuses.Remove(EBfkStatus::Shock);
		Emit(EBfkEvt::StatusApplied, Victim.Id, -1, -Shock, (int32)EBfkStatus::Shock);
		// chain to adjacent
		const int32 Chain = FMath::Max(1, Shock / 2) + (Config.Weather == EBfkWeather::Rain ? 1 : 0);
		for (FBfkUnitState& U : Units)
		{
			if (!U.bAlive || U.Id == Victim.Id) continue;
			if (Bfk::HexDist(U.Cell, Victim.Cell) == 1) DealDamage(nullptr, U, Chain, false);
		}
	}

	if (HazardAt(Victim.Cell) == EBfkHazard::VoidRift) Remaining += 1;

	// ward soaks first, then block
	int32 Blocked = 0;
	if (int32 WardStacks = Victim.Status(EBfkStatus::Ward))
	{
		const int32 Use = FMath::Min(WardStacks, Remaining);
		ApplyStatus(Victim, EBfkStatus::Ward, -Use);
		Remaining -= Use; Blocked += Use;
	}
	if (Victim.Block > 0 && Remaining > 0)
	{
		const int32 Use = FMath::Min(Victim.Block, Remaining);
		Victim.Block -= Use;
		Remaining -= Use; Blocked += Use;
	}

	Victim.Hp -= Remaining;
	Emit(EBfkEvt::UnitDamaged, Victim.Id, Attacker ? Attacker->Id : -1, Remaining, Blocked, 0, CardSlug);

	// thorns
	if (bIsAttack && Attacker && Attacker->bAlive && Victim.Status(EBfkStatus::Thorns) > 0)
	{
		const bool bMelee = Bfk::HexDist(Attacker->Cell, Victim.Cell) <= 1;
		if (bMelee) DealDamage(nullptr, *Attacker, Victim.Status(EBfkStatus::Thorns), false);
	}

	if (Victim.Hp > 0 && Victim.Hp <= Victim.MaxHp / 2)
	{
		RelicHook(EBfkRelicHook::OnHpBelowHalf, Victim);
	}
	if (Remaining > 0)
	{
		RelicHook(EBfkRelicHook::OnDamaged, Victim, Remaining);
	}

	if (Victim.Hp <= 0)
	{
		KillUnit(Victim, Attacker);
	}
}

void UBfkBattle::ApplyStatus(FBfkUnitState& U, EBfkStatus S, int32 Stacks)
{
	if (Stacks == 0 || !U.bAlive) return;
	int32& Cur = U.Statuses.FindOrAdd(S);
	const int32 Old = Cur;
	Cur = FMath::Max(0, Cur + Stacks);
	if (Cur == 0) U.Statuses.Remove(S);
	if (Cur != Old) Emit(EBfkEvt::StatusApplied, U.Id, -1, Cur - Old, (int32)S);
}

void UBfkBattle::HealUnit(FBfkUnitState& U, int32 Amount)
{
	if (!U.bAlive || Amount <= 0) return;
	if (U.Status(EBfkStatus::Curse) > 0)
	{
		Emit(EBfkEvt::UnitHealed, U.Id, -1, 0); // curse eats it (UI shows "cursed!")
		return;
	}
	if (Config.Weather == EBfkWeather::Ashfall) Amount = FMath::Max(0, Amount - 1);
	const int32 Healed = FMath::Min(Amount, U.MaxHp - U.Hp);
	U.Hp += Healed;
	Emit(EBfkEvt::UnitHealed, U.Id, -1, Healed);
}

void UBfkBattle::GainBlock(FBfkUnitState& U, int32 Amount)
{
	if (!U.bAlive) return;
	Amount = FMath::Max(0, Amount - U.Status(EBfkStatus::Rust));
	U.Block += Amount;
	Emit(EBfkEvt::UnitBlocked, U.Id, -1, Amount);
}

void UBfkBattle::MoveUnit(FBfkUnitState& U, const FBfkCell& To, bool bShoved)
{
	const FBfkCell From = U.Cell;
	U.Cell = To;
	Emit(EBfkEvt::UnitMoved, U.Id, -1, CellCode(From), CellCode(To), bShoved ? 1 : 0);

	const EBfkHazard H = HazardAt(To);
	if (H == EBfkHazard::PowderKeg && bShoved)
	{
		PlaceHazard(To, EBfkHazard::None);
		Emit(EBfkEvt::HazardTriggered, U.Id, -1, CellCode(To), (int32)EBfkHazard::PowderKeg);
		DealDamage(nullptr, U, kKegDamage, false);
	}
	else if (H == EBfkHazard::CrystalShard)
	{
		Emit(EBfkEvt::HazardTriggered, U.Id, -1, CellCode(To), (int32)EBfkHazard::CrystalShard);
		DealDamage(nullptr, U, 3, false);
	}
	else if (bShoved && H != EBfkHazard::None)
	{
		TickHazardsFor(U);
	}
}

void UBfkBattle::ShoveUnit(FBfkUnitState& Shover, FBfkUnitState& Target, int32 DeltaCol, int32 DeltaRow, int32 Dist)
{
	RelicHook(EBfkRelicHook::OnShove, Shover);
	for (int32 Step = 0; Step < Dist && Target.bAlive; ++Step)
	{
		FBfkCell Next(Target.Cell.Row + DeltaRow, Target.Cell.Col + DeltaCol);
		const int32 StepsLeft = Dist - Step;
		if (!Next.IsValid())
		{
			// wall slam
			Emit(EBfkEvt::Collision, Target.Id, -1, kCollisionDamage + StepsLeft - 1);
			DealDamage(nullptr, Target, kCollisionDamage + StepsLeft - 1, false);
			return;
		}
		if (FBfkUnitState* Blocker = MutUnitAt(Next))
		{
			const int32 Dmg = kCollisionDamage + StepsLeft - 1;
			Emit(EBfkEvt::Collision, Target.Id, Blocker->Id, Dmg);
			DealDamage(nullptr, Target, Dmg, false);
			DealDamage(nullptr, *Blocker, Dmg, false);
			return;
		}
		MoveUnit(Target, Next, true);
	}
}

void UBfkBattle::PlaceHazard(const FBfkCell& Cell, EBfkHazard H)
{
	if (!Cell.IsValid()) return;
	if (H == EBfkHazard::None) Hazards.Remove(Cell);
	else Hazards.Add(Cell, H);
	Emit(EBfkEvt::HazardChanged, -1, -1, CellCode(Cell), (int32)H);
}

void UBfkBattle::KillUnit(FBfkUnitState& U, FBfkUnitState* Killer)
{
	U.bAlive = false;
	U.Hp = 0;
	Emit(EBfkEvt::UnitDied, U.Id);
	SeverCards(U.Id);
	Intents.Remove(U.Id);
	if (Killer && Killer->bAlive)
	{
		RelicHook(EBfkRelicHook::OnKill, *Killer);
	}
}

void UBfkBattle::SeverCards(int32 UnitId)
{
	for (int32 S = 0; S < 2; ++S)
	{
		for (const FBfkCardInstance& C : Hand[S])
			if (C.OwnerUnitId == UnitId) Emit(EBfkEvt::CardSevered, -1, -1, C.InstanceId, S);
	}
}

void UBfkBattle::RestoreCards(int32 UnitId)
{
	for (int32 S = 0; S < 2; ++S)
	{
		for (const FBfkCardInstance& C : Hand[S])
			if (C.OwnerUnitId == UnitId) Emit(EBfkEvt::CardRestored, -1, -1, C.InstanceId, S);
	}
}

void UBfkBattle::SummonMinion(FName Species, bool bEnemySide, const FBfkCell& Prefer)
{
	const FBfkSpeciesDef* Sp = FBfkContent::Species(Species);
	if (!Sp) return;
	FBfkCell Cell = Prefer;
	if (!Cell.IsValid() || UnitAt(Cell) || (Cell.IsAllySide() == bEnemySide))
	{
		Cell = FindFreeCell(bEnemySide, Prefer.IsValid() ? Prefer.Row : 1);
	}
	if (!Cell.IsValid()) return;

	FBfkUnitState U;
	U.Id = NextUnitId++;
	U.Species = Species;
	U.Display = Sp->Display;
	U.bEnemySide = bEnemySide;
	U.MaxHp = Sp->MaxHp;
	U.Hp = U.MaxHp;
	U.Power = Sp->Power;
	U.bBeast = Sp->bBeast;
	U.bOriginal = false;
	U.Cell = Cell;
	U.Range = Bfk::ArchetypeRange(Sp->Archetype);
	U.Move = Bfk::ArchetypeMove(Sp->Archetype);
	Units.Add(U);
	Emit(EBfkEvt::UnitSummoned, U.Id);

	if (bEnemySide && !Config.bPvp)
	{
		// give it an intent immediately
		const FBfkSpeciesDef* Sp2 = FBfkContent::Species(Species);
		if (Sp2 && Sp2->SignatureCards.Num() > 0)
		{
			FIntent I;
			I.Card = Sp2->SignatureCards[Rng.RandRange(0, Sp2->SignatureCards.Num() - 1)];
			I.TargetCode = -1;
			const FBfkCardDef* D = Def(I.Card);
			if (D && (D->Target == EBfkTarget::Enemy || D->Target == EBfkTarget::EnemyFront))
			{
				const int32 Reach = D->bMeleeOnly ? 1 : U.Range;
				TArray<const FBfkUnitState*> Ps;
				for (const FBfkUnitState& T : Units)
					if (T.bAlive && !T.bEnemySide && Bfk::HexDist(U.Cell, T.Cell) <= Reach) Ps.Add(&T);
				if (Ps.Num()) I.TargetCode = CellCode(Ps[Rng.RandRange(0, Ps.Num() - 1)]->Cell);
				else { I.Card = NAME_None; I.bAdvance = true; }
			}
			else if (D && D->Target == EBfkTarget::Lane)
			{
				I.TargetCode = CellCode(FBfkCell(U.Cell.Row, 0));
			}
			Intents.Add(U.Id, I);
			Emit(EBfkEvt::IntentUpdated, U.Id, -1, I.TargetCode, 0, 0, I.Card);
		}
	}
}

void UBfkBattle::CheckEnd()
{
	if (bOver) return;
	const int32 AlliesAlive = CountAlive(false);
	const int32 EnemiesAlive = CountAlive(true);
	if (EnemiesAlive == 0)
	{
		bOver = true; bVictory = true;
		Emit(EBfkEvt::BattleEnded, -1, -1, 1, 0);
	}
	else if (AlliesAlive == 0)
	{
		bOver = true; bVictory = false;
		Emit(EBfkEvt::BattleEnded, -1, -1, 0, 1);
	}
}
