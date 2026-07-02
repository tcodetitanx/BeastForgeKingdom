#include "BfkGameInstance.h"
#include "Core/BfkContent.h"
#include "Core/BfkAssets.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameUserSettings.h"

void UBfkGameInstance::Init()
{
	Super::Init();
	FBfkContent::EnsureInit();
	Profile();
	ApplySettings();
}

UBfkSaveGame* UBfkGameInstance::Profile()
{
	if (!Save)
	{
		Save = Cast<UBfkSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName(), 0));
		if (!Save)
		{
			Save = Cast<UBfkSaveGame>(UGameplayStatics::CreateSaveGameObject(UBfkSaveGame::StaticClass()));
			FBfkMeta::InitFreshProfile(*Save);
			SaveProfile();
		}
	}
	return Save;
}

void UBfkGameInstance::SaveProfile()
{
	if (Save)
	{
		UGameplayStatics::SaveGameToSlot(Save, SlotName(), 0);
	}
}

void UBfkGameInstance::ResetProfile()
{
	Save = Cast<UBfkSaveGame>(UGameplayStatics::CreateSaveGameObject(UBfkSaveGame::StaticClass()));
	FBfkMeta::InitFreshProfile(*Save);
	SaveProfile();
}

// ============================================================== run flow

bool UBfkGameInstance::HasActiveRun() { return Profile()->Run.bActive; }
FBfkRunState& UBfkGameInstance::Run() { return Profile()->Run; }

void UBfkGameInstance::StartNewRun(const TArray<FBfkRunSquadMember>& Squad, int32 Seed)
{
	FBfkRunState& R = Profile()->Run;
	R = FBfkRunState();
	R.bActive = true;
	R.Seed = Seed;
	R.Act = 1;
	R.NodeId = -1;
	R.Map = FBfkRunLogic::GenerateMap(Seed, 1);
	R.Squad = Squad;
	R.Gold = 60;
	R.bBoonPending = true;
	Profile()->RunsStarted++;
	Shops.Reset();
	ForgedNodes.Reset();
	RestedNodes.Reset();
	SaveProfile();
}

void UBfkGameInstance::AbandonRun()
{
	Profile()->Run.bActive = false;
	SaveProfile();
}

TArray<int32> UBfkGameInstance::ReachableNodes() { return FBfkRunLogic::ReachableNodes(Run()); }

const FBfkMapNode* UBfkGameInstance::CurrentNode()
{
	FBfkRunState& R = Run();
	return R.Map.IsValidIndex(R.NodeId) ? &R.Map[R.NodeId] : nullptr;
}

const FBfkMapNode* UBfkGameInstance::Node(int32 Id)
{
	FBfkRunState& R = Run();
	return R.Map.IsValidIndex(Id) ? &R.Map[Id] : nullptr;
}

void UBfkGameInstance::MoveToNode(int32 Id)
{
	FBfkRunState& R = Run();
	if (!R.Map.IsValidIndex(Id)) return;
	R.NodeId = Id;
	R.Map[Id].bVisited = true;
	SaveProfile();
}

// ============================================================== battle hosting

UBfkBattle* UBfkGameInstance::StartNodeBattle()
{
	const FBfkMapNode* N = CurrentNode();
	if (!N) return nullptr;

	FBfkBattleConfig Cfg = FBfkRunLogic::MakeBattleConfig(Run(), *N, RunDeckExtras());

	// apply persistent squad hp + inherited cards from vault
	for (int32 i = 0; i < Cfg.Allies.Num() && i < Run().Squad.Num(); ++i)
	{
		if (FBfkOwnedBeast* B = FBfkMeta::FindBeast(*Profile(), Run().Squad[i].VaultId))
		{
			Cfg.Allies[i].BonusHp = B->BonusHp;
			Cfg.Allies[i].BonusPower = B->BonusPower;
			Cfg.Allies[i].InheritedCard = B->InheritedCard;
			Cfg.Allies[i].Display = B->Nickname;
		}
	}

	Battle = NewObject<UBfkBattle>(this);
	Battle->Init(Cfg);

	// run extras (drafted cards) join the player deck as clones owned by squad units
	// (handled inside battle deck already? No — merge here.)
	BattleCtx = EBfkBattleContext::RunNode;
	PendingNodeForBattle = N->Id;

	// enemy library
	for (const FBfkFighterSpec& E : Cfg.Enemies) Profile()->SeenEnemies.Add(E.Species);
	return Battle;
}

UBfkBattle* UBfkGameInstance::StartLocalBattle(const TArray<FBfkRunSquadMember>& Squad, const TArray<FName>& EnemySpecies, EBfkWeather Weather, int32 Seed)
{
	FBfkBattleConfig Cfg;
	Cfg.Seed = Seed;
	Cfg.Weather = Weather;
	Cfg.bAllowCapture = false;
	for (const FBfkRunSquadMember& M : Squad)
	{
		FBfkFighterSpec S;
		S.Species = M.Species; S.Weapon = M.Weapon; S.Relic = M.Relic; S.VaultId = M.VaultId;
		if (FBfkOwnedBeast* B = FBfkMeta::FindBeast(*Profile(), M.VaultId))
		{
			S.BonusHp = B->BonusHp; S.BonusPower = B->BonusPower; S.InheritedCard = B->InheritedCard;
		}
		Cfg.Allies.Add(S);
	}
	for (FName E : EnemySpecies)
	{
		FBfkFighterSpec S; S.Species = E;
		Cfg.Enemies.Add(S);
	}
	Battle = NewObject<UBfkBattle>(this);
	Battle->Init(Cfg);
	BattleCtx = EBfkBattleContext::LocalBattle;
	for (FName E : EnemySpecies) Profile()->SeenEnemies.Add(E);
	return Battle;
}

UBfkBattle* UBfkGameInstance::StartPvpBattle(const TArray<FBfkRunSquadMember>& SquadA, const TArray<FBfkRunSquadMember>& SquadB, EBfkWeather Weather, int32 Seed)
{
	FBfkBattleConfig Cfg;
	Cfg.Seed = Seed;
	Cfg.Weather = Weather;
	Cfg.bPvp = true;
	Cfg.bAllowCapture = false;
	auto Fill = [this](const TArray<FBfkRunSquadMember>& Squad, TArray<FBfkFighterSpec>& Out)
	{
		for (const FBfkRunSquadMember& M : Squad)
		{
			FBfkFighterSpec S;
			S.Species = M.Species; S.Weapon = M.Weapon; S.Relic = M.Relic; S.VaultId = M.VaultId;
			if (FBfkOwnedBeast* B = FBfkMeta::FindBeast(*Profile(), M.VaultId))
			{
				S.BonusHp = B->BonusHp; S.BonusPower = B->BonusPower; S.InheritedCard = B->InheritedCard;
			}
			Out.Add(S);
		}
	};
	Fill(SquadA, Cfg.Allies);
	Fill(SquadB, Cfg.Enemies);
	Battle = NewObject<UBfkBattle>(this);
	Battle->Init(Cfg);
	BattleCtx = EBfkBattleContext::Pvp;
	return Battle;
}

void UBfkGameInstance::ApplyBattleHpToSquad()
{
	if (!Battle) return;
	FBfkRunState& R = Run();
	for (const FBfkUnitState& U : Battle->GetUnits())
	{
		if (U.bEnemySide) continue;
		for (FBfkRunSquadMember& M : R.Squad)
		{
			if (M.VaultId == U.VaultId)
			{
				M.CurrentHp = FMath::Max(1, U.Hp); // fallen allies limp on at 1
			}
		}
	}
}

FBfkRewardBundle UBfkGameInstance::FinishBattle()
{
	FBfkRewardBundle Out;
	if (!Battle) return Out;

	const bool bVictory = Battle->WasVictory();

	if (BattleCtx == EBfkBattleContext::RunNode)
	{
		FBfkRunState& R = Run();
		const FBfkMapNode* N = Node(PendingNodeForBattle);
		FRandomStream Rng(N ? N->Seed * 31 : 7);

		if (bVictory && N)
		{
			R.BattlesWon++;
			if (N->Type == EBfkNode::Elite) R.ElitesSlain++;
			Out.Gold = FBfkRunLogic::GoldReward(N->Type, Rng);
			R.Gold += Out.Gold;

			// forgedust: the permanent leveling currency
			Out.Forgedust = N->Type == EBfkNode::Boss ? 45 : (N->Type == EBfkNode::Elite ? 20 : 12);
			Profile()->Forgedust += Out.Forgedust;

			// squad species for reward pool
			TArray<FName> SquadSpecies;
			for (const FBfkRunSquadMember& M : R.Squad) SquadSpecies.Add(M.Species);
			Out.CardChoices = FBfkContent::CardRewardChoices(SquadSpecies, Rng, 3, N->Type != EBfkNode::Battle);

			if (N->Type == EBfkNode::Elite || N->Type == EBfkNode::Boss)
			{
				TSet<FName> Owned = Profile()->OwnedRelics;
				TArray<FName> Drops = FBfkContent::RelicPool(Rng, 1, Owned);
				if (Drops.Num()) { Out.RelicDrop = Drops[0]; Profile()->OwnedRelics.Add(Drops[0]); }
				Out.Emberglass = N->Type == EBfkNode::Boss ? 3 : 1;
				Profile()->Emberglass += Out.Emberglass;
			}
			if (N->Type == EBfkNode::Elite && Rng.RandRange(1, 100) <= 45)
			{
				TSet<FName> Owned = Profile()->OwnedWeapons;
				TArray<FName> Drops = FBfkContent::WeaponPool(Rng, 1, R.Act, Owned);
				if (Drops.Num()) { Out.WeaponDrop = Drops[0]; Profile()->OwnedWeapons.Add(Drops[0]); }
			}

			// captures -> vault
			for (FName Species : Battle->CapturedThisBattle)
			{
				Profile()->Vault.Add(FBfkMeta::MakeBeast(Species));
				Profile()->TotalCaptures++;
				R.CapturesThisRun++;
				Out.Captured.Add(Species);
			}

			// eggs tick on victory
			Out.Hatched = FBfkMeta::TickEggs(*Profile());

			ApplyBattleHpToSquad();

			if (N->Type == EBfkNode::Boss)
			{
				R.bBossDefeated = true;
				Profile()->DeepestAct = FMath::Max(Profile()->DeepestAct, R.Act + 1);
			}
			Profile()->Soulshards += (N->Type == EBfkNode::Boss) ? 30 : 4;
		}
		else if (!bVictory)
		{
			// death: run ends, meta stays
			Profile()->Soulshards += 5 + R.BattlesWon * 2;
			R.bActive = false;
		}
		Out.Milestones = FBfkMeta::EvaluateMilestones(*Profile());
		SaveProfile();
	}
	else
	{
		// local/pvp: nothing persistent except enemy library
		SaveProfile();
	}

	Battle = nullptr;
	BattleCtx = EBfkBattleContext::None;
	PendingNodeForBattle = -1;
	return Out;
}

// ============================================================== rewards / shop / forge / rest

void UBfkGameInstance::TakeCardReward(FName Card)
{
	Run().ExtraCards.Add(Card);
	SaveProfile();
}

FBfkShopStock& UBfkGameInstance::ShopFor(const FBfkMapNode& NodeRef)
{
	if (FBfkShopStock* Existing = Shops.Find(NodeRef.Id)) return *Existing;

	FRandomStream Rng(NodeRef.Seed * 17 + 3);
	FBfkShopStock Stock;

	TArray<FName> SquadSpecies;
	for (const FBfkRunSquadMember& M : Run().Squad) SquadSpecies.Add(M.Species);
	Stock.Cards = FBfkContent::CardRewardChoices(SquadSpecies, Rng, 4, true);
	for (FName C : Stock.Cards)
	{
		const FBfkCardDef* D = FBfkContent::Card(C);
		const int32 Base = D ? (30 + (int32)D->Rarity * 22) : 40;
		Stock.CardPrices.Add(Base + Rng.RandRange(-5, 8));
	}
	TSet<FName> OwnedW = Profile()->OwnedWeapons;
	Stock.Weapons = FBfkContent::WeaponPool(Rng, 2, Run().Act, OwnedW);
	for (FName W : Stock.Weapons)
	{
		const FBfkWeaponDef* D = FBfkContent::Weapon(W);
		Stock.WeaponPrices.Add(D && D->Rarity == EBfkRarity::Legendary ? Rng.RandRange(140, 180) : Rng.RandRange(55, 80));
	}
	TSet<FName> OwnedR = Profile()->OwnedRelics;
	Stock.Relics = FBfkContent::RelicPool(Rng, 1, OwnedR);
	for (FName R : Stock.Relics)
	{
		Stock.RelicPrices.Add(Rng.RandRange(90, 130));
	}
	return Shops.Add(NodeRef.Id, Stock);
}

bool UBfkGameInstance::BuyShopCard(int32 Index)
{
	const FBfkMapNode* N = CurrentNode();
	if (!N) return false;
	FBfkShopStock& S = ShopFor(*N);
	if (!S.Cards.IsValidIndex(Index) || Run().Gold < S.CardPrices[Index]) return false;
	Run().Gold -= S.CardPrices[Index];
	Run().ExtraCards.Add(S.Cards[Index]);
	S.Cards.RemoveAt(Index); S.CardPrices.RemoveAt(Index);
	SaveProfile();
	return true;
}

bool UBfkGameInstance::BuyShopWeapon(int32 Index)
{
	const FBfkMapNode* N = CurrentNode();
	if (!N) return false;
	FBfkShopStock& S = ShopFor(*N);
	if (!S.Weapons.IsValidIndex(Index) || Run().Gold < S.WeaponPrices[Index]) return false;
	Run().Gold -= S.WeaponPrices[Index];
	Profile()->OwnedWeapons.Add(S.Weapons[Index]);
	S.Weapons.RemoveAt(Index); S.WeaponPrices.RemoveAt(Index);
	SaveProfile();
	return true;
}

bool UBfkGameInstance::BuyShopRelic(int32 Index)
{
	const FBfkMapNode* N = CurrentNode();
	if (!N) return false;
	FBfkShopStock& S = ShopFor(*N);
	if (!S.Relics.IsValidIndex(Index) || Run().Gold < S.RelicPrices[Index]) return false;
	Run().Gold -= S.RelicPrices[Index];
	Profile()->OwnedRelics.Add(S.Relics[Index]);
	S.Relics.RemoveAt(Index); S.RelicPrices.RemoveAt(Index);
	SaveProfile();
	return true;
}

bool UBfkGameInstance::BuyShopHeal()
{
	const FBfkMapNode* N = CurrentNode();
	if (!N) return false;
	FBfkShopStock& S = ShopFor(*N);
	const int32 Price = 45;
	if (S.bHealPurchased || Run().Gold < Price) return false;
	Run().Gold -= Price;
	S.bHealPurchased = true;
	for (FBfkRunSquadMember& M : Run().Squad) M.CurrentHp = -1; // full heal
	SaveProfile();
	return true;
}

bool UBfkGameInstance::ForgeUpgradeCard(FName Card)
{
	const FBfkMapNode* N = CurrentNode();
	if (!N || ForgedNodes.Contains(N->Id)) return false;
	ForgedNodes.Add(N->Id);
	int32& Count = Run().UpgradedCards.FindOrAdd(Card);
	Count++;
	SaveProfile();
	return true;
}

bool UBfkGameInstance::RestHeal()
{
	const FBfkMapNode* N = CurrentNode();
	if (!N || RestedNodes.Contains(N->Id)) return false;
	RestedNodes.Add(N->Id);
	for (FBfkRunSquadMember& M : Run().Squad)
	{
		if (M.CurrentHp > 0)
		{
			// heal 40% of species max
			const FBfkSpeciesDef* Sp = FBfkContent::Species(M.Species);
			const int32 Max = Sp ? Sp->MaxHp : 30;
			M.CurrentHp = FMath::Min(Max, M.CurrentHp + FMath::CeilToInt(Max * 0.4f));
		}
	}
	SaveProfile();
	return true;
}

bool UBfkGameInstance::RestHatch()
{
	const FBfkMapNode* N = CurrentNode();
	if (!N || RestedNodes.Contains(N->Id) || Profile()->Eggs.Num() == 0) return false;
	RestedNodes.Add(N->Id);
	for (FBfkEgg& E : Profile()->Eggs) E.BattlesToHatch = FMath::Max(1, E.BattlesToHatch - 1);
	SaveProfile();
	return true;
}

FString UBfkGameInstance::ResolveEvent(EBfkEventKind Kind, int32 Choice)
{
	const FBfkMapNode* N = CurrentNode();
	FRandomStream Rng(N ? N->Seed * 13 + Choice : Choice);
	FBfkRunState& R = Run();
	FString Result;

	switch (Kind)
	{
	case EBfkEventKind::WildEgg:
		if (Choice == 0 && Profile()->Emberglass >= 1)
		{
			Profile()->Emberglass -= 1;
			TArray<FName> Species = FBfkContent::CapturableSpecies(R.Act, Rng, 1);
			if (Species.Num())
			{
				FBfkEgg Egg;
				Egg.Id = FGuid::NewGuid();
				Egg.ResultSpecies = Species[0];
				Egg.Generation = 0;
				Egg.BattlesToHatch = 2;
				Egg.ParentNote = TEXT("Found in the wilds");
				Profile()->Eggs.Add(Egg);
				Result = TEXT("You warm the egg in your lantern's glow. Something shifts inside.");
			}
		}
		else Result = TEXT("You leave the nest be. The rain applauds politely.");
		break;
	case EBfkEventKind::CursedShrine:
		if (Choice == 0)
		{
			TSet<FName> Owned = Profile()->OwnedRelics;
			TArray<FName> Drop = FBfkContent::RelicPool(Rng, 1, Owned);
			if (Drop.Num())
			{
				Profile()->OwnedRelics.Add(Drop[0]);
				for (FBfkRunSquadMember& M : R.Squad)
				{
					if (M.CurrentHp > 8) M.CurrentHp -= 6;
				}
				const FBfkRelicDef* D = FBfkContent::Relic(Drop[0]);
				Result = FString::Printf(TEXT("The shrine takes its toll in blood and grants: %s."), D ? *D->Display : TEXT("a relic"));
			}
		}
		else Result = TEXT("You bow, and step away. Some bargains smell like the sea.");
		break;
	case EBfkEventKind::Castaway:
		if (Choice == 0)
		{
			const int32 Gold = Rng.RandRange(25, 45);
			R.Gold += Gold;
			Result = FString::Printf(TEXT("The castaway shares a rain-soaked purse: +%d gold."), Gold);
		}
		else
		{
			TArray<FName> SquadSpecies;
			for (const FBfkRunSquadMember& M : R.Squad) SquadSpecies.Add(M.Species);
			TArray<FName> Cards = FBfkContent::CardRewardChoices(SquadSpecies, Rng, 1, true);
			if (Cards.Num())
			{
				R.ExtraCards.Add(Cards[0]);
				const FBfkCardDef* D = FBfkContent::Card(Cards[0]);
				Result = FString::Printf(TEXT("The castaway teaches you: %s."), D ? *D->Display : TEXT("a technique"));
			}
		}
		break;
	case EBfkEventKind::RainTotem:
	{
		static const EBfkWeather Options[] = { EBfkWeather::Rain, EBfkWeather::Snow, EBfkWeather::Ashfall, EBfkWeather::Gloom };
		R.ForcedWeather = Options[FMath::Clamp(Choice, 0, 3)];
		R.bHasForcedWeather = true;
		Result = FString::Printf(TEXT("The totem drinks the sky. %s follows you now."), *Bfk::WeatherName(R.ForcedWeather));
		break;
	}
	case EBfkEventKind::Forgefire:
		Result = TEXT("Embers remember your name."); // handled by forge screen re-use
		break;
	case EBfkEventKind::TollBridge:
		if (Choice == 0 && R.Gold >= 30)
		{
			R.Gold -= 30;
			Result = TEXT("You pay the ferryman's cousin. He seems disappointed.");
		}
		else
		{
			for (FBfkRunSquadMember& M : R.Squad)
			{
				if (M.CurrentHp > 5) M.CurrentHp -= 4;
			}
			Result = TEXT("You wade the black water. It keeps a little of everyone.");
		}
		break;
	}
	SaveProfile();
	return Result;
}

bool UBfkGameInstance::AdvanceAct()
{
	FBfkRunState& R = Run();
	if (R.Act >= 4)
	{
		Profile()->RunsWon++;
		R.bActive = false;
		FBfkMeta::EvaluateMilestones(*Profile());
		SaveProfile();
		return false; // game complete
	}
	R.Act++;
	R.NodeId = -1;
	R.bBossDefeated = false;
	R.Map = FBfkRunLogic::GenerateMap(R.Seed, R.Act);
	R.bHasForcedWeather = false;
	Shops.Reset();
	ForgedNodes.Reset();
	RestedNodes.Reset();
	SaveProfile();
	return true;
}

void UBfkGameInstance::EquipWeapon(int32 SquadIdx, FName Weapon)
{
	if (Run().Squad.IsValidIndex(SquadIdx))
	{
		Run().Squad[SquadIdx].Weapon = Weapon;
		SaveProfile();
	}
}

void UBfkGameInstance::EquipRelic(int32 SquadIdx, FName Relic)
{
	if (Run().Squad.IsValidIndex(SquadIdx))
	{
		Run().Squad[SquadIdx].Relic = Relic;
		SaveProfile();
	}
}

void UBfkGameInstance::ApplySettings()
{
	const FBfkSettings& S = Profile()->Settings;
	if (UGameUserSettings* GUS = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		GUS->SetFullscreenMode(S.WindowMode == 2 ? EWindowMode::Fullscreen
			: S.WindowMode == 1 ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed);
		GUS->ApplySettings(false);
	}
}

float UBfkGameInstance::EffectiveSfxVolume()
{
	const FBfkSettings& S = Profile()->Settings;
	return S.MasterVolume * S.SfxVolume;
}

void UBfkGameInstance::UiSound(FName Slug, float Volume)
{
	FBfkAssets::PlayUi(this, Slug, Volume * EffectiveSfxVolume());
}
