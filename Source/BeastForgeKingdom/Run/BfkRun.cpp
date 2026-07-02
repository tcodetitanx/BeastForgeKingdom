#include "BfkRun.h"
#include "Core/BfkContent.h"

TArray<FBfkMapNode> FBfkRunLogic::GenerateMap(int32 Seed, int32 Act)
{
	FRandomStream Rng(Seed * 977 + Act * 131);
	TArray<FBfkMapNode> Map;

	const int32 Floors = 9;                       // 0..7 path, 8 = boss
	TArray<TArray<int32>> Rows;                   // node ids per floor
	int32 NextId = 0;

	for (int32 F = 0; F < Floors; ++F)
	{
		const bool bBossFloor = F == Floors - 1;
		const int32 Width = bBossFloor ? 1 : (F == 0 ? 2 : Rng.RandRange(2, 4));
		TArray<int32> Row;
		for (int32 i = 0; i < Width; ++i)
		{
			FBfkMapNode N;
			N.Id = NextId++;
			N.Floor = F;
			N.X = Width == 1 ? 0.5f : (float)i / (float)(Width - 1) * 0.8f + 0.1f
				+ (Width > 1 ? Rng.FRandRange(-0.05f, 0.05f) : 0.f);
			N.Seed = Rng.RandRange(1, INT32_MAX - 1);

			if (bBossFloor) N.Type = EBfkNode::Boss;
			else if (F == 0) N.Type = EBfkNode::Battle;
			else
			{
				const int32 R = Rng.RandRange(1, 100);
				if (F == Floors - 2)                 N.Type = R <= 60 ? EBfkNode::Rest : EBfkNode::Forge;
				else if (R <= 42)                    N.Type = EBfkNode::Battle;
				else if (R <= 56)                    N.Type = EBfkNode::Event;
				else if (R <= 68 && F >= 3)          N.Type = EBfkNode::Elite;
				else if (R <= 78)                    N.Type = EBfkNode::Shop;
				else if (R <= 88)                    N.Type = EBfkNode::Forge;
				else if (R <= 95)                    N.Type = EBfkNode::BreedingDen;
				else                                 N.Type = EBfkNode::Rest;
				if (R > 42 && R <= 68 && F < 3)      N.Type = EBfkNode::Battle;
			}
			Row.Add(N.Id);
			Map.Add(N);
		}
		Rows.Add(Row);
	}

	// connect floors: each node links to 1-2 nearest nodes on the next floor,
	// and every next-floor node keeps at least one inbound edge.
	for (int32 F = 0; F < Floors - 1; ++F)
	{
		const TArray<int32>& Cur = Rows[F];
		const TArray<int32>& Nxt = Rows[F + 1];
		TSet<int32> Inbound;
		for (int32 Id : Cur)
		{
			FBfkMapNode& N = Map[Id];
			// sort next floor by x proximity
			TArray<int32> Sorted = Nxt;
			Sorted.Sort([&Map, &N](int32 A, int32 B)
			{
				return FMath::Abs(Map[A].X - N.X) < FMath::Abs(Map[B].X - N.X);
			});
			const int32 Links = FMath::Min(Sorted.Num(), Rng.RandRange(1, 2));
			for (int32 i = 0; i < Links; ++i)
			{
				N.Next.AddUnique(Sorted[i]);
				Inbound.Add(Sorted[i]);
			}
		}
		for (int32 Id : Nxt)
		{
			if (!Inbound.Contains(Id))
			{
				// attach to nearest current node
				int32 Best = Cur[0];
				for (int32 C : Cur)
					if (FMath::Abs(Map[C].X - Map[Id].X) < FMath::Abs(Map[Best].X - Map[Id].X)) Best = C;
				Map[Best].Next.AddUnique(Id);
			}
		}
	}
	return Map;
}

TArray<int32> FBfkRunLogic::ReachableNodes(const FBfkRunState& Run)
{
	TArray<int32> Out;
	if (Run.NodeId < 0)
	{
		for (const FBfkMapNode& N : Run.Map) if (N.Floor == 0) Out.Add(N.Id);
	}
	else if (Run.Map.IsValidIndex(Run.NodeId))
	{
		Out = Run.Map[Run.NodeId].Next;
	}
	return Out;
}

FBfkBattleConfig FBfkRunLogic::MakeBattleConfig(const FBfkRunState& Run, const FBfkMapNode& Node,
                                                const TArray<FName>& RunDeckExtras)
{
	FRandomStream Rng(Node.Seed);
	FBfkBattleConfig Cfg;
	Cfg.Seed = Node.Seed;
	Cfg.Act = Run.Act;
	Cfg.Weather = Run.bHasForcedWeather ? Run.ForcedWeather : FBfkContent::RollWeather(Run.Act, Rng);

	for (const FBfkRunSquadMember& M : Run.Squad)
	{
		FBfkFighterSpec S;
		S.Species = M.Species;
		S.Weapon = M.Weapon;
		S.Relic = M.Relic;
		S.VaultId = M.VaultId;
		Cfg.Allies.Add(S);
	}

	TArray<FName> Foes;
	if (Node.Type == EBfkNode::Boss)
	{
		const FName Boss = FBfkContent::BossFor(Run.Act);
		Foes.Add(Boss);
		Foes.Append(FBfkContent::BossMinions(Boss));
	}
	else
	{
		Foes = FBfkContent::RollEncounter(Run.Act, Node.Type == EBfkNode::Elite, Rng);
	}

	const int32 HpScale = (Run.Act - 1) * 10 + (Node.Floor * 2);
	const int32 PowScale = (Run.Act - 1);
	for (FName F : Foes)
	{
		FBfkFighterSpec S;
		S.Species = F;
		const FBfkSpeciesDef* Sp = FBfkContent::Species(F);
		if (Sp && !Sp->bBoss)
		{
			S.BonusHp = HpScale;
			S.BonusPower = PowScale;
		}
		Cfg.Enemies.Add(S);
	}
	(void)RunDeckExtras; // extras merged by the battle host when building the deck
	return Cfg;
}

int32 FBfkRunLogic::GoldReward(EBfkNode Type, FRandomStream& Rng)
{
	switch (Type)
	{
	case EBfkNode::Elite: return Rng.RandRange(38, 55);
	case EBfkNode::Boss:  return Rng.RandRange(70, 95);
	default:              return Rng.RandRange(18, 30);
	}
}

EBfkEventKind FBfkRunLogic::EventFor(const FBfkMapNode& Node)
{
	return (EBfkEventKind)(Node.Seed % 6);
}
