// Headless battle-engine smoke tests: play many randomized battles to completion.
#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Battle/BfkBattle.h"
#include "Core/BfkContent.h"
#include "Run/BfkRun.h"
#include "Meta/BfkProfile.h"

#if WITH_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBfkContentSanityTest, "BeastForge.Content.Sanity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBfkContentSanityTest::RunTest(const FString&)
{
	FBfkContent::EnsureInit();

	TestTrue(TEXT("species registered"), FBfkContent::AllSpecies().Num() > 250);
	TestTrue(TEXT("cards registered"), FBfkContent::AllCards().Num() > 300);
	TestTrue(TEXT("weapons registered"), FBfkContent::AllWeapons().Num() > 100);
	TestTrue(TEXT("relics registered"), FBfkContent::AllRelics().Num() > 60);

	// every species has a kit whose cards all exist
	int32 MissingCards = 0;
	for (const auto& KV : FBfkContent::AllSpecies())
	{
		if (KV.Value.SignatureCards.Num() == 0)
		{
			AddError(FString::Printf(TEXT("species %s has no signature cards"), *KV.Key.ToString()));
		}
		for (FName C : KV.Value.SignatureCards)
		{
			if (!FBfkContent::Card(C)) { ++MissingCards; AddError(FString::Printf(TEXT("missing card %s (species %s)"), *C.ToString(), *KV.Key.ToString())); }
		}
	}
	TestEqual(TEXT("no dangling kit cards"), MissingCards, 0);

	// bosses exist for all acts
	for (int32 Act = 1; Act <= 4; ++Act)
	{
		const FBfkSpeciesDef* Boss = FBfkContent::Species(FBfkContent::BossFor(Act));
		TestNotNull(TEXT("boss exists"), Boss);
		if (Boss) TestTrue(TEXT("boss flagged"), Boss->bBoss);
	}

	// starter squad valid (full 5v5 roster)
	TArray<FName> Starters = FBfkContent::StarterSquad();
	TestEqual(TEXT("starter squad of 5"), Starters.Num(), Bfk::SquadSize);
	for (FName S : Starters) TestNotNull(TEXT("starter exists"), FBfkContent::Species(S));

	// weapon grants resolve
	for (const auto& KV : FBfkContent::AllWeapons())
	{
		for (FName C : KV.Value.GrantedCards)
		{
			if (!FBfkContent::Card(C)) AddError(FString::Printf(TEXT("weapon %s grants missing card %s"), *KV.Key.ToString(), *C.ToString()));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBfkBattleSimTest, "BeastForge.Battle.RandomSim",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBfkBattleSimTest::RunTest(const FString&)
{
	FBfkContent::EnsureInit();

	int32 Victories = 0, Defeats = 0;
	for (int32 Trial = 0; Trial < 60; ++Trial)
	{
		FRandomStream Rng(1000 + Trial * 7);
		FBfkBattleConfig Cfg;
		Cfg.Seed = 5000 + Trial;
		Cfg.Weather = (EBfkWeather)(Trial % 4);
		Cfg.Act = 1 + Trial % 4;

		for (FName S : FBfkContent::StarterSquad())
		{
			FBfkFighterSpec Spec;
			Spec.Species = S;
			Cfg.Allies.Add(Spec);
		}
		TArray<FName> Foes;
		if (Trial % 10 == 9)
		{
			const FName Boss = FBfkContent::BossFor(Cfg.Act);
			Foes.Add(Boss);
			Foes.Append(FBfkContent::BossMinions(Boss));
		}
		else
		{
			Foes = FBfkContent::RollEncounter(Cfg.Act, Trial % 5 == 4, Rng);
		}
		for (FName F : Foes)
		{
			FBfkFighterSpec Spec;
			Spec.Species = F;
			Cfg.Enemies.Add(Spec);
		}

		UBfkBattle* B = NewObject<UBfkBattle>();
		B->Init(Cfg);

		int32 Guard = 0;
		while (!B->IsOver() && ++Guard < 400)
		{
			// random voluntary moves for a couple of units (exercises hex movement)
			for (const FBfkUnitState& U : B->GetUnits())
			{
				if (B->IsOver()) break;
				if (!B->CanUnitMove(U.Id) || Rng.RandRange(0, 100) > 40) continue;
				TArray<int32> Cells = B->GetMoveCells(U.Id);
				if (Cells.Num()) B->MoveCommand(U.Id, Cells[Rng.RandRange(0, Cells.Num() - 1)]);
			}

			// play random playable cards until none playable, then end turn
			bool bPlayed = true;
			int32 PlayGuard = 0;
			while (bPlayed && !B->IsOver() && ++PlayGuard < 40)
			{
				bPlayed = false;
				const TArray<FBfkCardInstance> Hand = B->GetHand(B->GetActiveSide());
				for (const FBfkCardInstance& CI : Hand)
				{
					FString Why;
					if (!B->CanPlayCard(CI.InstanceId, Why)) continue;
					const FBfkCardDef* D = FBfkContent::Card(CI.CardSlug);
					if (!D) continue;
					int32 Target = -1;
					TArray<int32> Units = B->GetValidUnitTargets(CI.InstanceId);
					TArray<int32> Cells = B->GetValidCellTargets(CI.InstanceId);
					if (D->Target == EBfkTarget::None) Target = -1;
					else if (Units.Num()) Target = Units[Rng.RandRange(0, Units.Num() - 1)];
					else if (Cells.Num()) Target = Cells[Rng.RandRange(0, Cells.Num() - 1)];
					else continue;
					if (B->PlayCard(CI.InstanceId, Target)) { bPlayed = true; break; }
				}
			}
			if (!B->IsOver()) B->EndTurn(Rng.RandRange(0, 1) == 1);   // randomly keep or discard
			B->Events.Reset();
		}

		if (!B->IsOver())
		{
			AddError(FString::Printf(TEXT("battle %d did not terminate (guard hit)"), Trial));
		}
		else
		{
			if (B->WasVictory()) ++Victories; else ++Defeats;
		}

		// invariants
		for (const FBfkUnitState& U : B->GetUnits())
		{
			TestTrue(TEXT("hp not negative-crazy"), U.Hp >= -50);
			if (U.bAlive) TestTrue(TEXT("alive units on valid cells"), U.Cell.IsValid());
		}
	}
	AddInfo(FString::Printf(TEXT("random sims: %d victories / %d defeats"), Victories, Defeats));
	TestTrue(TEXT("some battles are winnable"), Victories > 0);
	TestTrue(TEXT("some battles are losable (bosses bite)"), Defeats > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBfkRunMapTest, "BeastForge.Run.MapGen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBfkRunMapTest::RunTest(const FString&)
{
	for (int32 Seed = 1; Seed <= 25; ++Seed)
	{
		for (int32 Act = 1; Act <= 4; ++Act)
		{
			TArray<FBfkMapNode> Map = FBfkRunLogic::GenerateMap(Seed, Act);
			TestTrue(TEXT("map has nodes"), Map.Num() >= 10);

			// exactly one boss, on the last floor
			int32 Bosses = 0, MaxFloor = 0;
			for (const FBfkMapNode& N : Map)
			{
				MaxFloor = FMath::Max(MaxFloor, N.Floor);
				if (N.Type == EBfkNode::Boss) ++Bosses;
			}
			TestEqual(TEXT("single boss"), Bosses, 1);

			// every node below the top floor has at least one outgoing edge,
			// and all edges point to the next floor
			for (const FBfkMapNode& N : Map)
			{
				if (N.Floor < MaxFloor)
				{
					TestTrue(TEXT("node has outgoing edge"), N.Next.Num() > 0);
					for (int32 Nx : N.Next)
					{
						TestTrue(TEXT("edge target valid"), Map.IsValidIndex(Nx));
						TestEqual(TEXT("edge goes up one floor"), Map[Nx].Floor, N.Floor + 1);
					}
				}
			}

			// boss reachable from floor 0 via BFS
			TSet<int32> Visited;
			TArray<int32> Queue;
			for (const FBfkMapNode& N : Map) if (N.Floor == 0) { Queue.Add(N.Id); Visited.Add(N.Id); }
			while (Queue.Num())
			{
				const int32 Cur = Queue.Pop();
				for (int32 Nx : Map[Cur].Next)
				{
					if (!Visited.Contains(Nx)) { Visited.Add(Nx); Queue.Add(Nx); }
				}
			}
			bool bBossReached = false;
			for (const FBfkMapNode& N : Map) if (N.Type == EBfkNode::Boss && Visited.Contains(N.Id)) bBossReached = true;
			TestTrue(TEXT("boss reachable"), bBossReached);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBfkBreedingTest, "BeastForge.Meta.Breeding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBfkBreedingTest::RunTest(const FString&)
{
	FBfkContent::EnsureInit();
	UBfkSaveGame* Save = NewObject<UBfkSaveGame>();
	FBfkMeta::InitFreshProfile(*Save);
	TestEqual(TEXT("starter vault"), Save->Vault.Num(), Bfk::SquadSize);

	// forgedust leveling
	{
		Save->Forgedust = 100;
		const FGuid Id = Save->Vault[0].Id;
		const int32 Hp0 = Save->Vault[0].BonusHp;
		FString Why;
		TestTrue(TEXT("level up succeeds"), FBfkMeta::LevelUpBeast(*Save, Id, Why));
		TestEqual(TEXT("level tracked"), Save->Vault[0].Level, 1);
		TestEqual(TEXT("hp bumped"), Save->Vault[0].BonusHp, Hp0 + 2);
		TestTrue(TEXT("dust spent"), Save->Forgedust < 100);
		Save->Forgedust = 0;
		TestFalse(TEXT("level up blocked when broke"), FBfkMeta::LevelUpBeast(*Save, Id, Why));
	}

	// find two compatible beasts among all species and vault them
	FName A = NAME_None, Bn = NAME_None;
	for (const auto& K1 : FBfkContent::AllSpecies())
	{
		if (!K1.Value.bBeast || K1.Value.bEnemyOnly) continue;
		for (const auto& K2 : FBfkContent::AllSpecies())
		{
			if (!K2.Value.bBeast || K2.Value.bEnemyOnly || K1.Key == K2.Key) continue;
			if (K1.Value.Element == K2.Value.Element)
			{
				A = K1.Key; Bn = K2.Key;
				break;
			}
		}
		if (!A.IsNone()) break;
	}
	TestFalse(TEXT("found compatible pair"), A.IsNone());

	FBfkOwnedBeast BeastA = FBfkMeta::MakeBeast(A);
	FBfkOwnedBeast BeastB = FBfkMeta::MakeBeast(Bn);
	Save->Vault.Add(BeastA);
	Save->Vault.Add(BeastB);
	Save->Emberglass = 10;

	FString Why;
	TestTrue(FString::Printf(TEXT("can breed (%s)"), *Why), FBfkMeta::CanBreed(*Save, BeastA.Id, BeastB.Id, Why));
	FBfkEgg Egg = FBfkMeta::Breed(*Save, BeastA.Id, BeastB.Id, 42);
	TestTrue(TEXT("egg produced"), Egg.Id.IsValid());
	TestFalse(TEXT("egg species set"), Egg.ResultSpecies.IsNone());
	TestEqual(TEXT("emberglass spent"), Save->Emberglass, 10 - FBfkMeta::BreedCost());

	// eggs hatch after victories
	int32 HatchedTotal = 0;
	for (int32 i = 0; i < 5; ++i)
	{
		HatchedTotal += FBfkMeta::TickEggs(*Save).Num();
	}
	TestEqual(TEXT("egg hatched"), HatchedTotal, 1);
	return true;
}

#endif // WITH_AUTOMATION_TESTS
