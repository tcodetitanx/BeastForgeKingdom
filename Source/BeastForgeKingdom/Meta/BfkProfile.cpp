#include "BfkProfile.h"
#include "Core/BfkContent.h"

const TArray<FBfkMilestone>& FBfkMeta::Milestones()
{
	static TArray<FBfkMilestone> M;
	if (M.Num() == 0)
	{
		auto Add = [](FName Id, const FString& Display, const FString& Desc, FName Relic, FName Weapon, int32 Shards)
		{
			FBfkMilestone Ms;
			Ms.Id = Id; Ms.Display = Display; Ms.Desc = Desc;
			Ms.RewardRelic = Relic; Ms.RewardWeapon = Weapon; Ms.RewardShards = Shards;
			return Ms;
		};
		M.Add(Add(TEXT("first-capture"), TEXT("First Snare"), TEXT("Capture your first wild beast."), NAME_None, NAME_None, 20));
		M.Add(Add(TEXT("five-captures"), TEXT("Beastcaller"), TEXT("Capture 5 wild beasts."), NAME_None, NAME_None, 40));
		M.Add(Add(TEXT("first-breed"), TEXT("Hearthborn"), TEXT("Hatch your first bred egg."), NAME_None, NAME_None, 40));
		M.Add(Add(TEXT("act1-clear"), TEXT("Wilds Walker"), TEXT("Defeat the Rot Shepherd."), NAME_None, NAME_None, 60));
		M.Add(Add(TEXT("act2-clear"), TEXT("Wharf Reaver"), TEXT("Defeat Ghostwake."), NAME_None, NAME_None, 80));
		M.Add(Add(TEXT("act3-clear"), TEXT("Hollow Light"), TEXT("Defeat the Facet Queen."), NAME_None, NAME_None, 100));
		M.Add(Add(TEXT("game-clear"), TEXT("Forgewarden"), TEXT("Defeat the Tidebound King."), NAME_None, NAME_None, 200));
		M.Add(Add(TEXT("ten-elites"), TEXT("Elite Hunter"), TEXT("Slay 10 elites across all runs."), NAME_None, NAME_None, 60));
		M.Add(Add(TEXT("twenty-vault"), TEXT("Full Kennels"), TEXT("Hold 20 beasts in the vault."), NAME_None, NAME_None, 80));
		M.Add(Add(TEXT("gen-three"), TEXT("Bloodline"), TEXT("Hatch a generation-3 beast."), NAME_None, NAME_None, 120));
	}
	return M;
}

void FBfkMeta::InitFreshProfile(UBfkSaveGame& Save)
{
	FBfkContent::EnsureInit();
	for (FName Species : FBfkContent::StarterSquad())
	{
		Save.Vault.Add(MakeBeast(Species));
	}
	// a couple of starter weapons so the squad screen has gear day one
	int32 Added = 0;
	TArray<FName> Names;
	FBfkContent::AllWeapons().GenerateKeyArray(Names);
	Names.Sort([](const FName& L, const FName& R){ return L.LexicalLess(R); });
	for (FName W : Names)
	{
		const FBfkWeaponDef* D = FBfkContent::Weapon(W);
		if (D && D->Rarity == EBfkRarity::Common)
		{
			Save.OwnedWeapons.Add(W);
			if (++Added >= 2) break;
		}
	}
	Save.Emberglass = 3;
	Save.Soulshards = 0;
}

FBfkOwnedBeast* FBfkMeta::FindBeast(UBfkSaveGame& Save, const FGuid& Id)
{
	for (FBfkOwnedBeast& B : Save.Vault) if (B.Id == Id) return &B;
	return nullptr;
}

bool FBfkMeta::LevelUpBeast(UBfkSaveGame& Save, const FGuid& Id, FString& WhyNot)
{
	FBfkOwnedBeast* B = FindBeast(Save, Id);
	if (!B) { WhyNot = TEXT("No such beast."); return false; }
	const int32 Cost = LevelUpCost(B->Level);
	if (Save.Forgedust < Cost)
	{
		WhyNot = FString::Printf(TEXT("Needs %d Forgedust."), Cost);
		return false;
	}
	Save.Forgedust -= Cost;
	B->Level++;
	B->BonusHp += 2;
	B->BonusPower += 1;
	return true;
}

int32 FBfkMeta::SacrificeValue(const FBfkOwnedBeast& B, int32 Which)
{
	const FBfkSpeciesDef* Sp = FBfkContent::Species(B.Species);
	const int32 Quality = Sp ? Sp->Quality : 1;
	// worth scales with how much you've invested (quality, level, generation)
	const int32 Worth = Quality * 2 + B.Level + B.Generation;
	switch (Which)
	{
	case 1:  return FMath::Max(1, Worth / 3);      // Emberglass (rare)
	case 2:  return 10 + Worth * 6;                // Forgedust
	default: return 8 + Worth * 4;                 // Soulshards
	}
}

bool FBfkMeta::SacrificeBeast(UBfkSaveGame& Save, const FGuid& Id, int32 Which, int32& OutAmount, FString& WhyNot)
{
	const FBfkOwnedBeast* B = FindBeast(Save, Id);
	if (!B) { WhyNot = TEXT("No such beast."); return false; }
	// don't let the vault be emptied below a playable squad
	int32 Beasts = 0;
	for (const FBfkOwnedBeast& O : Save.Vault)
	{
		const FBfkSpeciesDef* Sp = FBfkContent::Species(O.Species);
		if (Sp && Sp->bBeast) ++Beasts;
	}
	const FBfkSpeciesDef* MySp = FBfkContent::Species(B->Species);
	if (MySp && MySp->bBeast && Beasts <= 3)
	{
		WhyNot = TEXT("You must keep at least three beasts for a squad.");
		return false;
	}

	OutAmount = SacrificeValue(*B, Which);
	switch (Which)
	{
	case 1:  Save.Emberglass += OutAmount; break;
	case 2:  Save.Forgedust  += OutAmount; break;
	default: Save.Soulshards += OutAmount; break;
	}
	Save.Vault.RemoveAll([&Id](const FBfkOwnedBeast& O){ return O.Id == Id; });
	return true;
}

FGuid FBfkMeta::FindDuplicate(const UBfkSaveGame& Save, const FGuid& Id)
{
	const FBfkOwnedBeast* B = nullptr;
	for (const FBfkOwnedBeast& O : Save.Vault) if (O.Id == Id) { B = &O; break; }
	if (!B) return FGuid();
	for (const FBfkOwnedBeast& O : Save.Vault)
	{
		if (O.Id != Id && O.Species == B->Species) return O.Id;
	}
	return FGuid();
}

bool FBfkMeta::MergeBeasts(UBfkSaveGame& Save, const FGuid& Keep, const FGuid& Consume, FString& WhyNot)
{
	if (Keep == Consume) { WhyNot = TEXT("Pick two different beasts."); return false; }
	FBfkOwnedBeast* K = FindBeast(Save, Keep);
	FBfkOwnedBeast* C = FindBeast(Save, Consume);
	if (!K || !C) { WhyNot = TEXT("Missing beast."); return false; }
	if (K->Species != C->Species) { WhyNot = TEXT("Merge needs two of the SAME creature."); return false; }

	// fold the consumed one in: it makes the survivor meaningfully stronger and
	// carries over the better of the two on every axis.
	K->BonusHp    = FMath::Max(K->BonusHp, C->BonusHp)       + 4 + C->Level;
	K->BonusPower = FMath::Max(K->BonusPower, C->BonusPower) + 2;
	K->Level      = FMath::Max(K->Level, C->Level) + 1;
	K->Generation = FMath::Max(K->Generation, C->Generation);
	if (K->InheritedCard.IsNone() && !C->InheritedCard.IsNone()) K->InheritedCard = C->InheritedCard;
	K->Victories += C->Victories;

	Save.Vault.RemoveAll([&Consume](const FBfkOwnedBeast& O){ return O.Id == Consume; });
	return true;
}

FBfkOwnedBeast FBfkMeta::MakeBeast(FName Species, int32 Generation)
{
	FBfkOwnedBeast B;
	B.Id = FGuid::NewGuid();
	B.Species = Species;
	B.Generation = Generation;
	return B;
}

bool FBfkMeta::CanBreed(const UBfkSaveGame& Save, const FGuid& A, const FGuid& B, FString& WhyNot)
{
	if (A == B) { WhyNot = TEXT("A beast cannot breed with itself."); return false; }
	const FBfkOwnedBeast* PA = nullptr; const FBfkOwnedBeast* PB = nullptr;
	for (const FBfkOwnedBeast& Beast : Save.Vault)
	{
		if (Beast.Id == A) PA = &Beast;
		if (Beast.Id == B) PB = &Beast;
	}
	if (!PA || !PB) { WhyNot = TEXT("Missing parent."); return false; }
	const FBfkSpeciesDef* SA = FBfkContent::Species(PA->Species);
	const FBfkSpeciesDef* SB = FBfkContent::Species(PB->Species);
	if (!SA || !SB || !SA->bBeast || !SB->bBeast)
	{
		WhyNot = TEXT("Only beasts may enter the Breeding Chambers.");
		return false;
	}
	if (SA->Element != SB->Element && SA->Archetype != SB->Archetype)
	{
		WhyNot = TEXT("Parents must share an element or a temperament.");
		return false;
	}
	if (Save.Emberglass < BreedCost())
	{
		WhyNot = FString::Printf(TEXT("Maren wants %d Emberglass. The fire doesn't feed itself."), BreedCost());
		return false;
	}
	return true;
}

FBfkEgg FBfkMeta::Breed(UBfkSaveGame& Save, const FGuid& A, const FGuid& B, int32 Seed)
{
	FBfkEgg Egg;
	FString Why;
	if (!CanBreed(Save, A, B, Why)) return Egg;

	FBfkOwnedBeast* PA = FindBeast(Save, A);
	FBfkOwnedBeast* PB = FindBeast(Save, B);
	FRandomStream Rng(Seed);

	const FName Result = FBfkContent::BreedResult(PA->Species, PB->Species, Rng);
	if (Result.IsNone()) return Egg;

	Save.Emberglass -= BreedCost();
	Egg.Id = FGuid::NewGuid();
	Egg.ResultSpecies = Result;
	Egg.Generation = FMath::Max(PA->Generation, PB->Generation) + 1;
	Egg.BattlesToHatch = 3;

	// inheritance: blend of parents' bonuses + fresh mutation
	Egg.BonusHp = (PA->BonusHp + PB->BonusHp) / 2 + Rng.RandRange(2, 6);
	Egg.BonusPower = FMath::Max(PA->BonusPower, PB->BonusPower) + (Rng.RandRange(1, 100) <= 30 ? 1 : 0);

	// chance to inherit a mutated signature card from a parent's species
	if (Rng.RandRange(1, 100) <= 65)
	{
		const FBfkSpeciesDef* Donor = FBfkContent::Species(Rng.RandRange(0, 1) ? PA->Species : PB->Species);
		if (Donor && Donor->SignatureCards.Num())
		{
			Egg.InheritedCard = Donor->SignatureCards[Rng.RandRange(0, Donor->SignatureCards.Num() - 1)];
		}
	}
	const FBfkSpeciesDef* SA = FBfkContent::Species(PA->Species);
	const FBfkSpeciesDef* SB = FBfkContent::Species(PB->Species);
	Egg.ParentNote = FString::Printf(TEXT("%s x %s"),
		SA ? *SA->Display : TEXT("?"), SB ? *SB->Display : TEXT("?"));

	Save.Eggs.Add(Egg);
	Save.TotalBreeds++;
	return Egg;
}

TArray<FBfkOwnedBeast> FBfkMeta::TickEggs(UBfkSaveGame& Save)
{
	TArray<FBfkOwnedBeast> Hatched;
	for (int32 i = Save.Eggs.Num() - 1; i >= 0; --i)
	{
		FBfkEgg& Egg = Save.Eggs[i];
		if (--Egg.BattlesToHatch <= 0)
		{
			FBfkOwnedBeast B = MakeBeast(Egg.ResultSpecies, Egg.Generation);
			B.BonusHp = Egg.BonusHp;
			B.BonusPower = Egg.BonusPower;
			B.InheritedCard = Egg.InheritedCard;
			B.ParentNote = Egg.ParentNote;
			Save.Vault.Add(B);
			Hatched.Add(B);
			Save.Eggs.RemoveAt(i);
		}
	}
	return Hatched;
}

TArray<FBfkMilestone> FBfkMeta::EvaluateMilestones(UBfkSaveGame& Save)
{
	TArray<FBfkMilestone> New;
	auto Done = [&Save](FName Id) { return Save.CompletedMilestones.Contains(Id); };
	auto Check = [&](FName Id, bool bCond)
	{
		if (bCond && !Done(Id))
		{
			for (const FBfkMilestone& M : Milestones())
			{
				if (M.Id == Id)
				{
					Save.CompletedMilestones.Add(Id);
					Save.Soulshards += M.RewardShards;
					if (!M.RewardRelic.IsNone()) Save.UnlockedRelics.Add(M.RewardRelic);
					if (!M.RewardWeapon.IsNone()) Save.UnlockedWeapons.Add(M.RewardWeapon);
					New.Add(M);
				}
			}
		}
	};

	int32 MaxGen = 0;
	for (const FBfkOwnedBeast& B : Save.Vault) MaxGen = FMath::Max(MaxGen, B.Generation);

	Check(TEXT("first-capture"), Save.TotalCaptures >= 1);
	Check(TEXT("five-captures"), Save.TotalCaptures >= 5);
	Check(TEXT("first-breed"), Save.TotalBreeds >= 1 && MaxGen >= 1);
	Check(TEXT("act1-clear"), Save.DeepestAct >= 2);
	Check(TEXT("act2-clear"), Save.DeepestAct >= 3);
	Check(TEXT("act3-clear"), Save.DeepestAct >= 4);
	Check(TEXT("game-clear"), Save.RunsWon >= 1);
	Check(TEXT("ten-elites"), Save.Run.ElitesSlain >= 10 || Save.CompletedMilestones.Contains(TEXT("ten-elites")));
	Check(TEXT("twenty-vault"), Save.Vault.Num() >= 20);
	Check(TEXT("gen-three"), MaxGen >= 3);
	return New;
}
