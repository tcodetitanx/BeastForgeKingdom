// BeastForge Kingdom — hand-authored content + registry plumbing.
#include "Core/BfkContent.h"
#include "Battle/BfkBattle.h"

extern FName GAliasSporeling;
extern FName GAliasDeckhand;

bool FBfkContent::bInit = false;

namespace
{
	TMap<FName, FBfkSpeciesDef> GSpecies;
	TArray<FName> GSpeciesOrder;
	TMap<FName, FBfkCardDef> GCards;
	TMap<FName, FBfkRelicDef> GRelics;
	TMap<FName, FBfkWeaponDef> GWeapons;

	FBfkCardDef MakeCard(FName Slug, const FString& Display, int32 Cost, EBfkCardKind Kind,
	                     EBfkRarity Rarity, EBfkElement Elem, EBfkTarget Target,
	                     std::initializer_list<FBfkEffect> Fx, const FString& Text = FString())
	{
		FBfkCardDef C;
		C.Slug = Slug; C.Display = Display; C.Cost = Cost; C.Kind = Kind; C.Rarity = Rarity;
		C.Element = Elem; C.Target = Target; C.Text = Text;
		for (const FBfkEffect& F : Fx) C.Effects.Add(F);
		return C;
	}
}

void FBfkContent::AddSpecies(const FBfkSpeciesDef& S)
{
	if (!GSpecies.Contains(S.Slug)) GSpeciesOrder.Add(S.Slug);
	GSpecies.Add(S.Slug, S);
}
void FBfkContent::AddCard(const FBfkCardDef& C) { GCards.Add(C.Slug, C); }
void FBfkContent::AddRelic(const FBfkRelicDef& R) { GRelics.Add(R.Slug, R); }
void FBfkContent::AddWeapon(const FBfkWeaponDef& W) { GWeapons.Add(W.Slug, W); }

const TMap<FName, FBfkSpeciesDef>& FBfkContent::AllSpecies() { EnsureInit(); return GSpecies; }
const TMap<FName, FBfkCardDef>& FBfkContent::AllCards() { EnsureInit(); return GCards; }
const TMap<FName, FBfkRelicDef>& FBfkContent::AllRelics() { EnsureInit(); return GRelics; }
const TMap<FName, FBfkWeaponDef>& FBfkContent::AllWeapons() { EnsureInit(); return GWeapons; }

const FBfkSpeciesDef* FBfkContent::Species(FName Slug) { EnsureInit(); return GSpecies.Find(Slug); }
const FBfkCardDef* FBfkContent::Card(FName Slug) { EnsureInit(); return GCards.Find(Slug); }
const FBfkRelicDef* FBfkContent::Relic(FName Slug) { EnsureInit(); return GRelics.Find(Slug); }
const FBfkWeaponDef* FBfkContent::Weapon(FName Slug) { EnsureInit(); return GWeapons.Find(Slug); }

void FBfkContent::EnsureInit()
{
	if (bInit) return;
	bInit = true;
	InitGenerated();   // species/spells/weapons/relic fallbacks from art labels
	InitCore();        // hand-authored kits patch the generated species
	InitDerived();     // template kits for everyone else + weapon grants + specials
}

// ============================================================== hand-authored

void FBfkContent::InitCore()
{
	// ---------- neutral utility ----------
	AddCard(MakeCard(TEXT("soul-snare"), TEXT("Soul Snare"), 1, EBfkCardKind::Skill,
		EBfkRarity::Starter, EBfkElement::Arcane, EBfkTarget::Enemy,
		{ FBfkEffect(EBfkOp::Capture, 1), FBfkEffect(EBfkOp::ExhaustSelf, 0) },
		TEXT("Attempt to capture a wild beast. Better odds at low health. Once per battle.")));

	AddCard(MakeCard(TEXT("second-wind"), TEXT("Second Wind"), 1, EBfkCardKind::Skill,
		EBfkRarity::Common, EBfkElement::Neutral, EBfkTarget::Ally,
		{ FBfkEffect(EBfkOp::Heal, 6) }));
	AddCard(MakeCard(TEXT("reposition"), TEXT("Reposition"), 0, EBfkCardKind::Skill,
		EBfkRarity::Common, EBfkElement::Neutral, EBfkTarget::AllySlot,
		{ FBfkEffect(EBfkOp::MoveSelf, 0), FBfkEffect(EBfkOp::Draw, 1) }));
	AddCard(MakeCard(TEXT("brace"), TEXT("Brace"), 1, EBfkCardKind::Skill,
		EBfkRarity::Common, EBfkElement::Neutral, EBfkTarget::Ally,
		{ FBfkEffect::Blk(8) }));
	AddCard(MakeCard(TEXT("shield-slam"), TEXT("Shield Slam"), 2, EBfkCardKind::Attack,
		EBfkRarity::Uncommon, EBfkElement::Iron, EBfkTarget::EnemyFront,
		{ FBfkEffect::Dmg(7), FBfkEffect(EBfkOp::Push, 2) }));
	AddCard(MakeCard(TEXT("adrenal-surge"), TEXT("Adrenal Surge"), 0, EBfkCardKind::Skill,
		EBfkRarity::Rare, EBfkElement::Neutral, EBfkTarget::None,
		{ FBfkEffect(EBfkOp::Energy, 2), FBfkEffect(EBfkOp::ExhaustSelf, 0) }));

	// ---------- protagonist kits ----------
	auto Hero = [](FName Slug, std::initializer_list<FBfkCardDef> Cards)
	{
		TArray<FName> Sig;
		for (const FBfkCardDef& C : Cards) { FBfkContent::AddCard(C); Sig.Add(C.Slug); }
		if (FBfkSpeciesDef* Sp = GSpecies.Find(Slug)) Sp->SignatureCards = Sig; // may run before generated: patched in InitDerived
	};

	Hero(TEXT("captain-blacktide"), {
		MakeCard(TEXT("boarding-hook"), TEXT("Boarding Hook"), 1, EBfkCardKind::Attack, EBfkRarity::Starter, EBfkElement::Iron, EBfkTarget::Enemy,
			{ FBfkEffect::Dmg(5), FBfkEffect(EBfkOp::Pull, 1) }),
		MakeCard(TEXT("broadside"), TEXT("Broadside"), 2, EBfkCardKind::Attack, EBfkRarity::Starter, EBfkElement::Iron, EBfkTarget::Lane,
			{ FBfkEffect(EBfkOp::DamageLane, 11) }),
		MakeCard(TEXT("powder-keg-toss"), TEXT("Powder Keg"), 1, EBfkCardKind::Skill, EBfkRarity::Starter, EBfkElement::Ember, EBfkTarget::Cell,
			{ FBfkEffect::Hz(EBfkHazard::PowderKeg) }),
		MakeCard(TEXT("captains-orders"), TEXT("Captain's Orders"), 2, EBfkCardKind::Skill, EBfkRarity::Starter, EBfkElement::Neutral, EBfkTarget::None,
			{ FBfkEffect{EBfkOp::StatusSelfSide, 2, EBfkStatus::Rally}, FBfkEffect(EBfkOp::Draw, 1) }),
		MakeCard(TEXT("cutlass-flurry"), TEXT("Cutlass Flurry"), 1, EBfkCardKind::Attack, EBfkRarity::Starter, EBfkElement::Iron, EBfkTarget::EnemyFront,
			{ FBfkEffect::Dmg(4), FBfkEffect::Dmg(4) }),
	});

	Hero(TEXT("plague-warden"), {
		MakeCard(TEXT("censer-swing"), TEXT("Censer Swing"), 1, EBfkCardKind::Attack, EBfkRarity::Starter, EBfkElement::Verdant, EBfkTarget::Enemy,
			{ FBfkEffect::Dmg(4), FBfkEffect::St(EBfkStatus::Poison, 3) }),
		MakeCard(TEXT("miasma"), TEXT("Miasma"), 2, EBfkCardKind::Skill, EBfkRarity::Starter, EBfkElement::Verdant, EBfkTarget::None,
			{ FBfkEffect{EBfkOp::StatusAll, 2, EBfkStatus::Poison} }),
		MakeCard(TEXT("quarantine"), TEXT("Quarantine"), 1, EBfkCardKind::Skill, EBfkRarity::Starter, EBfkElement::Verdant, EBfkTarget::Ally,
			{ FBfkEffect::Blk(6), FBfkEffect(EBfkOp::Cleanse, 3) }),
		MakeCard(TEXT("virulent-bloom"), TEXT("Virulent Bloom"), 2, EBfkCardKind::Skill, EBfkRarity::Starter, EBfkElement::Verdant, EBfkTarget::Cell,
			{ FBfkEffect::Hz(EBfkHazard::Spores) }),
		MakeCard(TEXT("mercy-draught"), TEXT("Mercy Draught"), 1, EBfkCardKind::Skill, EBfkRarity::Starter, EBfkElement::Verdant, EBfkTarget::Ally,
			{ FBfkEffect(EBfkOp::Heal, 8), FBfkEffect(EBfkOp::DamageSelf, 2) }),
	});

	Hero(TEXT("plate-champion"), {
		MakeCard(TEXT("shield-wall"), TEXT("Shield Wall"), 2, EBfkCardKind::Skill, EBfkRarity::Starter, EBfkElement::Iron, EBfkTarget::None,
			{ FBfkEffect{EBfkOp::BlockAll, 6} }),
		MakeCard(TEXT("pommel-strike"), TEXT("Pommel Strike"), 1, EBfkCardKind::Attack, EBfkRarity::Starter, EBfkElement::Iron, EBfkTarget::EnemyFront,
			{ FBfkEffect::Dmg(6), FBfkEffect(EBfkOp::Push, 1) }),
		MakeCard(TEXT("bulwark"), TEXT("Bulwark"), 1, EBfkCardKind::Skill, EBfkRarity::Starter, EBfkElement::Iron, EBfkTarget::None,
			{ FBfkEffect::Blk(7), FBfkEffect::St(EBfkStatus::Thorns, 2) }),
		MakeCard(TEXT("crusaders-advance"), TEXT("Crusader's Advance"), 1, EBfkCardKind::Attack, EBfkRarity::Starter, EBfkElement::Iron, EBfkTarget::Enemy,
			{ FBfkEffect::Dmg(5), FBfkEffect::Blk(4) }),
		MakeCard(TEXT("unbreakable"), TEXT("Unbreakable"), 2, EBfkCardKind::Skill, EBfkRarity::Starter, EBfkElement::Iron, EBfkTarget::Ally,
			{ FBfkEffect::St(EBfkStatus::Ward, 6) }),
	});

	Hero(TEXT("antler-ranger"), {
		MakeCard(TEXT("piercing-arrow"), TEXT("Piercing Arrow"), 1, EBfkCardKind::Attack, EBfkRarity::Starter, EBfkElement::Verdant, EBfkTarget::Lane,
			{ FBfkEffect(EBfkOp::DamageLane, 8) }),
		MakeCard(TEXT("twin-shot"), TEXT("Twin Shot"), 2, EBfkCardKind::Attack, EBfkRarity::Starter, EBfkElement::Verdant, EBfkTarget::Enemy,
			{ FBfkEffect::Dmg(5), FBfkEffect::Dmg(5) }),
		MakeCard(TEXT("hunters-mark"), TEXT("Hunter's Mark"), 0, EBfkCardKind::Skill, EBfkRarity::Starter, EBfkElement::Storm, EBfkTarget::Enemy,
			{ FBfkEffect::St(EBfkStatus::Shock, 3) }),
		MakeCard(TEXT("thicket-trap"), TEXT("Thicket Trap"), 1, EBfkCardKind::Skill, EBfkRarity::Starter, EBfkElement::Verdant, EBfkTarget::Cell,
			{ FBfkEffect::Hz(EBfkHazard::Spores) }),
		MakeCard(TEXT("fall-back"), TEXT("Fall Back"), 0, EBfkCardKind::Skill, EBfkRarity::Starter, EBfkElement::Neutral, EBfkTarget::AllySlot,
			{ FBfkEffect(EBfkOp::MoveSelf, 0), FBfkEffect::Blk(3) }),
	});

	Hero(TEXT("swamp-witch"), {
		MakeCard(TEXT("hex-bolt"), TEXT("Hex Bolt"), 1, EBfkCardKind::Attack, EBfkRarity::Starter, EBfkElement::Shadow, EBfkTarget::Enemy,
			{ FBfkEffect::Dmg(6), FBfkEffect::St(EBfkStatus::Curse, 1) }),
		MakeCard(TEXT("witchfire"), TEXT("Witchfire"), 2, EBfkCardKind::Attack, EBfkRarity::Starter, EBfkElement::Ember, EBfkTarget::Enemy,
			{ FBfkEffect::Dmg(7), FBfkEffect::St(EBfkStatus::Burn, 3) }),
		MakeCard(TEXT("bog-brew"), TEXT("Bog Brew"), 2, EBfkCardKind::Skill, EBfkRarity::Starter, EBfkElement::Verdant, EBfkTarget::None,
			{ FBfkEffect{EBfkOp::HealAll, 4} }),
		MakeCard(TEXT("cauldron-toss"), TEXT("Cauldron Toss"), 1, EBfkCardKind::Skill, EBfkRarity::Starter, EBfkElement::Ember, EBfkTarget::Cell,
			{ FBfkEffect::Hz(EBfkHazard::Coals) }),
		MakeCard(TEXT("soul-siphon"), TEXT("Soul Siphon"), 2, EBfkCardKind::Attack, EBfkRarity::Starter, EBfkElement::Shadow, EBfkTarget::Enemy,
			{ FBfkEffect::Dmg(6), FBfkEffect(EBfkOp::Heal, 4) }),
	});

	Hero(TEXT("rat-cartographer"), {
		MakeCard(TEXT("dagger-toss"), TEXT("Dagger Toss"), 0, EBfkCardKind::Attack, EBfkRarity::Starter, EBfkElement::Iron, EBfkTarget::Enemy,
			{ FBfkEffect::Dmg(4) }),
		MakeCard(TEXT("scurry"), TEXT("Scurry"), 0, EBfkCardKind::Skill, EBfkRarity::Starter, EBfkElement::Neutral, EBfkTarget::AllySlot,
			{ FBfkEffect(EBfkOp::MoveSelf, 0), FBfkEffect(EBfkOp::Draw, 1) }),
		MakeCard(TEXT("mapped-weakness"), TEXT("Mapped Weakness"), 1, EBfkCardKind::Skill, EBfkRarity::Starter, EBfkElement::Arcane, EBfkTarget::Enemy,
			{ FBfkEffect::St(EBfkStatus::Shock, 2), FBfkEffect(EBfkOp::Draw, 1) }),
		MakeCard(TEXT("smoke-bomb"), TEXT("Smoke Bomb"), 1, EBfkCardKind::Skill, EBfkRarity::Starter, EBfkElement::Shadow, EBfkTarget::Ally,
			{ FBfkEffect::St(EBfkStatus::Stealth, 1), FBfkEffect::Blk(4) }),
		MakeCard(TEXT("pilfer"), TEXT("Pilfer"), 1, EBfkCardKind::Attack, EBfkRarity::Starter, EBfkElement::Neutral, EBfkTarget::EnemyFront,
			{ FBfkEffect::Dmg(5), FBfkEffect(EBfkOp::Energy, 1) }),
	});

	Hero(TEXT("crystal-assassin"), {
		MakeCard(TEXT("facet-blade"), TEXT("Facet Blade"), 1, EBfkCardKind::Attack, EBfkRarity::Starter, EBfkElement::Arcane, EBfkTarget::EnemyFront,
			{ FBfkEffect::Dmg(9) }),
		MakeCard(TEXT("shardstep"), TEXT("Shardstep"), 1, EBfkCardKind::Skill, EBfkRarity::Starter, EBfkElement::Arcane, EBfkTarget::AllySlot,
			{ FBfkEffect(EBfkOp::MoveSelf, 0), FBfkEffect::St(EBfkStatus::Stealth, 1) }),
		MakeCard(TEXT("prism-volley"), TEXT("Prism Volley"), 2, EBfkCardKind::Attack, EBfkRarity::Starter, EBfkElement::Arcane, EBfkTarget::None,
			{ FBfkEffect{EBfkOp::DamageAll, 5} }),
		MakeCard(TEXT("crystalline-edge"), TEXT("Crystalline Edge"), 1, EBfkCardKind::Skill, EBfkRarity::Starter, EBfkElement::Arcane, EBfkTarget::None,
			{ FBfkEffect::St(EBfkStatus::Rally, 2), FBfkEffect(EBfkOp::Draw, 1) }),
		MakeCard(TEXT("glass-rose"), TEXT("Glass Rose"), 2, EBfkCardKind::Skill, EBfkRarity::Starter, EBfkElement::Arcane, EBfkTarget::Cell,
			{ FBfkEffect::Hz(EBfkHazard::CrystalShard) }),
	});

	Hero(TEXT("chain-warlock"), {
		MakeCard(TEXT("chain-lash"), TEXT("Chain Lash"), 1, EBfkCardKind::Attack, EBfkRarity::Starter, EBfkElement::Shadow, EBfkTarget::Enemy,
			{ FBfkEffect::Dmg(5), FBfkEffect(EBfkOp::Pull, 1) }),
		MakeCard(TEXT("soul-chains"), TEXT("Soul Chains"), 1, EBfkCardKind::Skill, EBfkRarity::Starter, EBfkElement::Shadow, EBfkTarget::Enemy,
			{ FBfkEffect::St(EBfkStatus::Chill, 2), FBfkEffect::St(EBfkStatus::Curse, 1) }),
		MakeCard(TEXT("umbral-pact"), TEXT("Umbral Pact"), 0, EBfkCardKind::Skill, EBfkRarity::Starter, EBfkElement::Shadow, EBfkTarget::None,
			{ FBfkEffect(EBfkOp::Draw, 2), FBfkEffect(EBfkOp::DamageSelf, 3) }),
		MakeCard(TEXT("void-rift"), TEXT("Void Rift"), 1, EBfkCardKind::Skill, EBfkRarity::Starter, EBfkElement::Shadow, EBfkTarget::Cell,
			{ FBfkEffect::Hz(EBfkHazard::VoidRift) }),
		MakeCard(TEXT("dark-feast"), TEXT("Dark Feast"), 2, EBfkCardKind::Attack, EBfkRarity::Starter, EBfkElement::Shadow, EBfkTarget::Enemy,
			{ FBfkEffect::Dmg(8), FBfkEffect(EBfkOp::Heal, 4) }),
	});

	// ---------- boss kits ----------
	auto BossKit = [](FName Slug, std::initializer_list<FBfkCardDef> Cards)
	{
		TArray<FName> Sig;
		for (const FBfkCardDef& C : Cards) { FBfkContent::AddCard(C); Sig.Add(C.Slug); }
		if (FBfkSpeciesDef* Sp = GSpecies.Find(Slug)) Sp->SignatureCards = Sig;
	};

	BossKit(TEXT("rot-shepherd"), {
		MakeCard(TEXT("crook-of-rot"), TEXT("Crook of Rot"), 1, EBfkCardKind::Attack, EBfkRarity::Rare, EBfkElement::Verdant, EBfkTarget::Enemy,
			{ FBfkEffect::Dmg(8), FBfkEffect::St(EBfkStatus::Poison, 3) }),
		MakeCard(TEXT("spore-bloom"), TEXT("Spore Bloom"), 1, EBfkCardKind::Skill, EBfkRarity::Rare, EBfkElement::Verdant, EBfkTarget::Cell,
			{ FBfkEffect::Hz(EBfkHazard::Spores) }),
		MakeCard(TEXT("shepherds-wrath"), TEXT("Shepherd's Wrath"), 2, EBfkCardKind::Attack, EBfkRarity::Rare, EBfkElement::Verdant, EBfkTarget::None,
			{ FBfkEffect{EBfkOp::DamageAll, 6}, FBfkEffect{EBfkOp::StatusAll, 1, EBfkStatus::Poison} }),
	});

	BossKit(TEXT("ghostwake"), {
		MakeCard(TEXT("anchor-swing"), TEXT("Anchor Swing"), 1, EBfkCardKind::Attack, EBfkRarity::Rare, EBfkElement::Iron, EBfkTarget::Enemy,
			{ FBfkEffect::Dmg(10), FBfkEffect(EBfkOp::Push, 1) }),
		MakeCard(TEXT("rake-the-deck"), TEXT("Rake the Deck"), 2, EBfkCardKind::Attack, EBfkRarity::Rare, EBfkElement::Storm, EBfkTarget::Lane,
			{ FBfkEffect{EBfkOp::DamageRow, 8} }),
		MakeCard(TEXT("dead-mans-volley"), TEXT("Dead Man's Volley"), 2, EBfkCardKind::Attack, EBfkRarity::Rare, EBfkElement::Storm, EBfkTarget::None,
			{ FBfkEffect{EBfkOp::DamageAll, 7} }),
	});

	BossKit(TEXT("facet-queen"), {
		MakeCard(TEXT("shard-lance"), TEXT("Shard Lance"), 1, EBfkCardKind::Attack, EBfkRarity::Rare, EBfkElement::Arcane, EBfkTarget::Lane,
			{ FBfkEffect{EBfkOp::DamageLane, 12} }),
		MakeCard(TEXT("refracting-ray"), TEXT("Refracting Ray"), 1, EBfkCardKind::Attack, EBfkRarity::Rare, EBfkElement::Arcane, EBfkTarget::Enemy,
			{ FBfkEffect::Dmg(7), FBfkEffect::St(EBfkStatus::Shock, 3) }),
		MakeCard(TEXT("prismatic-scream"), TEXT("Prismatic Scream"), 2, EBfkCardKind::Attack, EBfkRarity::Rare, EBfkElement::Frost, EBfkTarget::None,
			{ FBfkEffect{EBfkOp::DamageAll, 6}, FBfkEffect{EBfkOp::StatusAll, 1, EBfkStatus::Chill} }),
	});

	BossKit(TEXT("tidebound-king"), {
		MakeCard(TEXT("anchor-chain-slam"), TEXT("Anchor-Chain Slam"), 2, EBfkCardKind::Attack, EBfkRarity::Legendary, EBfkElement::Iron, EBfkTarget::Enemy,
			{ FBfkEffect::Dmg(13), FBfkEffect(EBfkOp::Push, 2) }),
		MakeCard(TEXT("drowning-grasp"), TEXT("Drowning Grasp"), 1, EBfkCardKind::Attack, EBfkRarity::Legendary, EBfkElement::Frost, EBfkTarget::Enemy,
			{ FBfkEffect::Dmg(7), FBfkEffect(EBfkOp::Pull, 2), FBfkEffect::St(EBfkStatus::Chill, 2) }),
		MakeCard(TEXT("kings-grief"), TEXT("The King's Grief"), 2, EBfkCardKind::Attack, EBfkRarity::Legendary, EBfkElement::Shadow, EBfkTarget::None,
			{ FBfkEffect{EBfkOp::DamageAll, 8}, FBfkEffect{EBfkOp::StatusAll, 1, EBfkStatus::Curse} }),
	});

	// ---------- special relics (override generated fallbacks by slug) ----------
	auto SpecialRelic = [](FName Slug, const FString& Display, const FString& Desc, EBfkRarity R,
	                       EBfkRelicHook Hook, std::initializer_list<FBfkEffect> Fx,
	                       int32 Hp, int32 Pw, int32 En, const FString& UnlockHint = FString())
	{
		// keep the generated sprite if present
		FName Sprite;
		if (const FBfkRelicDef* Old = GRelics.Find(Slug)) Sprite = Old->SpriteSlug;
		FBfkRelicDef D;
		D.Slug = Slug; D.Display = Display; D.Desc = Desc; D.Rarity = R; D.SpriteSlug = Sprite;
		D.Hook = Hook; D.HpMod = Hp; D.PowerMod = Pw; D.EnergyMod = En; D.UnlockHint = UnlockHint;
		for (const FBfkEffect& F : Fx) D.HookEffects.Add(F);
		FBfkContent::AddRelic(D);
	};
	// NOTE: these run again in InitDerived (after generated relics register their sprites).
	(void)SpecialRelic;
}

// ============================================================== derived / template kits

void FBfkContent::InitDerived()
{
	// ---------- archetype x element kit cards ----------
	static const TCHAR* ElemAdj[] = { TEXT("Cinder"), TEXT("Hoarfrost"), TEXT("Briar"), TEXT("Galestorm"), TEXT("Umbral"), TEXT("Ironclad"), TEXT("Runeglow"), TEXT("Gray") };
	static const EBfkElement Elems[] = { EBfkElement::Ember, EBfkElement::Frost, EBfkElement::Verdant, EBfkElement::Storm, EBfkElement::Shadow, EBfkElement::Iron, EBfkElement::Arcane, EBfkElement::Neutral };
	static const EBfkStatus ElemStatus[] = { EBfkStatus::Burn, EBfkStatus::Chill, EBfkStatus::Poison, EBfkStatus::Shock, EBfkStatus::Curse, EBfkStatus::Rust, EBfkStatus::Rally, EBfkStatus::Ward };

	struct FArchKit { EBfkArchetype A; const TCHAR* Atk; const TCHAR* Skill; };
	static const FArchKit Arch[] = {
		{ EBfkArchetype::Bruiser,   TEXT("Slam"),    TEXT("Rampage") },
		{ EBfkArchetype::Tank,      TEXT("Bash"),    TEXT("Bulwark") },
		{ EBfkArchetype::Striker,   TEXT("Rend"),    TEXT("Lunge") },
		{ EBfkArchetype::Caster,    TEXT("Bolt"),    TEXT("Sigil") },
		{ EBfkArchetype::Support,   TEXT("Chime"),   TEXT("Blessing") },
		{ EBfkArchetype::Trickster, TEXT("Sting"),   TEXT("Feint") },
	};

	for (int32 Ei = 0; Ei < 8; ++Ei)
	{
		for (const FArchKit& K : Arch)
		{
			const FString ElemName(ElemAdj[Ei]);
			const FString ArchName = Bfk::ArchetypeName(K.A).ToLower();
			const FName SlugA(*FString::Printf(TEXT("kit-%s-%s-a"), *ElemName.ToLower(), *ArchName));
			const FName SlugB(*FString::Printf(TEXT("kit-%s-%s-b"), *ElemName.ToLower(), *ArchName));
			const FName SlugC(*FString::Printf(TEXT("kit-%s-%s-c"), *ElemName.ToLower(), *ArchName));
			const EBfkElement E = Elems[Ei];
			const EBfkStatus S = ElemStatus[Ei];

			switch (K.A)
			{
			case EBfkArchetype::Bruiser:
				AddCard(MakeCard(SlugA, ElemName + TEXT(" ") + K.Atk, 1, EBfkCardKind::Attack, EBfkRarity::Common, E, EBfkTarget::EnemyFront,
					{ FBfkEffect::Dmg(7), FBfkEffect(EBfkOp::Push, 1) }));
				AddCard(MakeCard(SlugB, ElemName + TEXT(" ") + K.Skill, 2, EBfkCardKind::Attack, EBfkRarity::Common, E, EBfkTarget::Lane,
					{ FBfkEffect{EBfkOp::DamageLane, 10} }));
				AddCard(MakeCard(SlugC, ElemName + TEXT(" Fury"), 1, EBfkCardKind::Skill, EBfkRarity::Common, E, EBfkTarget::None,
					{ FBfkEffect::St(EBfkStatus::Rally, 2), FBfkEffect::Blk(3) }));
				break;
			case EBfkArchetype::Tank:
				AddCard(MakeCard(SlugA, ElemName + TEXT(" ") + K.Atk, 1, EBfkCardKind::Attack, EBfkRarity::Common, E, EBfkTarget::EnemyFront,
					{ FBfkEffect::Dmg(5), FBfkEffect::Blk(4) }));
				AddCard(MakeCard(SlugB, ElemName + TEXT(" ") + K.Skill, 1, EBfkCardKind::Skill, EBfkRarity::Common, E, EBfkTarget::None,
					{ FBfkEffect::Blk(8) }));
				AddCard(MakeCard(SlugC, ElemName + TEXT(" Aegis"), 2, EBfkCardKind::Skill, EBfkRarity::Common, E, EBfkTarget::Ally,
					{ FBfkEffect::Blk(6), FBfkEffect::St(S, 2) }));
				break;
			case EBfkArchetype::Striker:
				AddCard(MakeCard(SlugA, ElemName + TEXT(" ") + K.Atk, 1, EBfkCardKind::Attack, EBfkRarity::Common, E, EBfkTarget::EnemyFront,
					{ FBfkEffect::Dmg(9) }));
				AddCard(MakeCard(SlugB, ElemName + TEXT(" ") + K.Skill, 2, EBfkCardKind::Attack, EBfkRarity::Common, E, EBfkTarget::Enemy,
					{ FBfkEffect::Dmg(6), FBfkEffect::St(S, 2) }));
				AddCard(MakeCard(SlugC, ElemName + TEXT(" Instinct"), 0, EBfkCardKind::Skill, EBfkRarity::Common, E, EBfkTarget::None,
					{ FBfkEffect(EBfkOp::Draw, 1), FBfkEffect::St(EBfkStatus::Rally, 1) }));
				break;
			case EBfkArchetype::Caster:
				AddCard(MakeCard(SlugA, ElemName + TEXT(" ") + K.Atk, 1, EBfkCardKind::Attack, EBfkRarity::Common, E, EBfkTarget::Enemy,
					{ FBfkEffect::Dmg(6), FBfkEffect::St(S, 2) }));
				AddCard(MakeCard(SlugB, ElemName + TEXT(" ") + K.Skill, 2, EBfkCardKind::Attack, EBfkRarity::Common, E, EBfkTarget::None,
					{ FBfkEffect{EBfkOp::DamageAll, 4}, FBfkEffect{EBfkOp::StatusAll, 1, S} }));
				AddCard(MakeCard(SlugC, ElemName + TEXT(" Focus"), 1, EBfkCardKind::Skill, EBfkRarity::Common, E, EBfkTarget::None,
					{ FBfkEffect(EBfkOp::Draw, 2) }));
				break;
			case EBfkArchetype::Support:
				AddCard(MakeCard(SlugA, ElemName + TEXT(" ") + K.Atk, 1, EBfkCardKind::Attack, EBfkRarity::Common, E, EBfkTarget::Enemy,
					{ FBfkEffect::Dmg(4), FBfkEffect{EBfkOp::Heal, 3} }));
				AddCard(MakeCard(SlugB, ElemName + TEXT(" ") + K.Skill, 2, EBfkCardKind::Skill, EBfkRarity::Common, E, EBfkTarget::None,
					{ FBfkEffect{EBfkOp::HealAll, 4} }));
				AddCard(MakeCard(SlugC, ElemName + TEXT(" Mending"), 1, EBfkCardKind::Skill, EBfkRarity::Common, E, EBfkTarget::Ally,
					{ FBfkEffect{EBfkOp::Heal, 5}, FBfkEffect(EBfkOp::Cleanse, 2) }));
				break;
			case EBfkArchetype::Trickster:
				AddCard(MakeCard(SlugA, ElemName + TEXT(" ") + K.Atk, 1, EBfkCardKind::Attack, EBfkRarity::Common, E, EBfkTarget::Enemy,
					{ FBfkEffect::Dmg(5), FBfkEffect(EBfkOp::Pull, 1) }));
				AddCard(MakeCard(SlugB, ElemName + TEXT(" ") + K.Skill, 1, EBfkCardKind::Skill, EBfkRarity::Common, E, EBfkTarget::AllySlot,
					{ FBfkEffect(EBfkOp::MoveSelf, 0), FBfkEffect(EBfkOp::Draw, 1) }));
				AddCard(MakeCard(SlugC, ElemName + TEXT(" Ruse"), 1, EBfkCardKind::Skill, EBfkRarity::Common, E, EBfkTarget::Ally,
					{ FBfkEffect::St(EBfkStatus::Stealth, 1), FBfkEffect::Blk(3) }));
				break;
			}
		}
	}

	// ---------- weapon class kit cards ----------
	struct FWKit { const TCHAR* WClass; const TCHAR* NameA; const TCHAR* NameB; };
	static const FWKit WKits[] = {
		{ TEXT("sword"),  TEXT("Cleave"),        TEXT("Riposte") },
		{ TEXT("axe"),    TEXT("Sundering Chop"),TEXT("Reckless Swing") },
		{ TEXT("mace"),   TEXT("Skull Ring"),    TEXT("Concussion") },
		{ TEXT("dagger"), TEXT("Backstab"),      TEXT("Quick Slice") },
		{ TEXT("spear"),  TEXT("Lance Thrust"),  TEXT("Phalanx") },
		{ TEXT("bow"),    TEXT("Longshot"),      TEXT("Pinning Arrow") },
		{ TEXT("gun"),    TEXT("Point Blank"),   TEXT("Suppressing Fire") },
		{ TEXT("staff"),  TEXT("Arc Burst"),     TEXT("Channel") },
		{ TEXT("wand"),   TEXT("Zap"),           TEXT("Glimmer") },
		{ TEXT("shield"), TEXT("Shield Charge"), TEXT("Turtle Up") },
		{ TEXT("tome"),   TEXT("Recite"),        TEXT("Deep Study") },
		{ TEXT("instrument"), TEXT("War Chord"), TEXT("Soothing Air") },
		{ TEXT("whip"),   TEXT("Lashing Reach"), TEXT("Disarm") },
		{ TEXT("hammer"), TEXT("Earthbreaker"),  TEXT("Anvil Guard") },
		{ TEXT("scythe"), TEXT("Reap"),          TEXT("Harvest") },
		{ TEXT("claw"),   TEXT("Shred"),         TEXT("Savage Flurry") },
		{ TEXT("orb"),    TEXT("Orb Pulse"),     TEXT("Scrying Light") },
		{ TEXT("other"),  TEXT("Improvise"),     TEXT("Heave") },
	};
	for (const FWKit& W : WKits)
	{
		const FName A(*FString::Printf(TEXT("wkit-%s-a"), W.WClass));
		const FName B(*FString::Printf(TEXT("wkit-%s-b"), W.WClass));
		const FString C(W.WClass);
		if (C == TEXT("sword"))
		{
			AddCard(MakeCard(A, W.NameA, 1, EBfkCardKind::Attack, EBfkRarity::Common, EBfkElement::Iron, EBfkTarget::EnemyFront, { FBfkEffect::Dmg(7) }));
			AddCard(MakeCard(B, W.NameB, 1, EBfkCardKind::Skill, EBfkRarity::Common, EBfkElement::Iron, EBfkTarget::None, { FBfkEffect::Blk(4), FBfkEffect::St(EBfkStatus::Thorns, 2) }));
		}
		else if (C == TEXT("axe") || C == TEXT("hammer") || C == TEXT("mace"))
		{
			AddCard(MakeCard(A, W.NameA, 2, EBfkCardKind::Attack, EBfkRarity::Common, EBfkElement::Iron, EBfkTarget::EnemyFront, { FBfkEffect::Dmg(11), FBfkEffect(EBfkOp::Push, 1) }));
			AddCard(MakeCard(B, W.NameB, 1, EBfkCardKind::Attack, EBfkRarity::Common, EBfkElement::Iron, EBfkTarget::EnemyFront, { FBfkEffect::Dmg(8), FBfkEffect(EBfkOp::DamageSelf, 2) }));
		}
		else if (C == TEXT("dagger") || C == TEXT("claw"))
		{
			AddCard(MakeCard(A, W.NameA, 0, EBfkCardKind::Attack, EBfkRarity::Common, EBfkElement::Iron, EBfkTarget::EnemyFront, { FBfkEffect::Dmg(5) }));
			AddCard(MakeCard(B, W.NameB, 1, EBfkCardKind::Attack, EBfkRarity::Common, EBfkElement::Iron, EBfkTarget::Enemy, { FBfkEffect::Dmg(3), FBfkEffect::Dmg(3), FBfkEffect(EBfkOp::Draw, 1) }));
		}
		else if (C == TEXT("spear") || C == TEXT("whip"))
		{
			AddCard(MakeCard(A, W.NameA, 1, EBfkCardKind::Attack, EBfkRarity::Common, EBfkElement::Iron, EBfkTarget::Lane, { FBfkEffect{EBfkOp::DamageLane, 8} }));
			AddCard(MakeCard(B, W.NameB, 1, EBfkCardKind::Skill, EBfkRarity::Common, EBfkElement::Iron, EBfkTarget::Enemy, { FBfkEffect::Dmg(4), FBfkEffect(EBfkOp::Pull, 1) }));
		}
		else if (C == TEXT("bow") || C == TEXT("gun"))
		{
			AddCard(MakeCard(A, W.NameA, 1, EBfkCardKind::Attack, EBfkRarity::Common, EBfkElement::Iron, EBfkTarget::Enemy, { FBfkEffect::Dmg(7) }));
			AddCard(MakeCard(B, W.NameB, 2, EBfkCardKind::Attack, EBfkRarity::Common, EBfkElement::Iron, EBfkTarget::Lane, { FBfkEffect{EBfkOp::DamageLane, 6}, FBfkEffect{EBfkOp::StatusAll, 0, EBfkStatus::Shock} }));
		}
		else if (C == TEXT("staff") || C == TEXT("wand") || C == TEXT("orb"))
		{
			AddCard(MakeCard(A, W.NameA, 1, EBfkCardKind::Attack, EBfkRarity::Common, EBfkElement::Arcane, EBfkTarget::Enemy, { FBfkEffect::Dmg(6), FBfkEffect::St(EBfkStatus::Shock, 1) }));
			AddCard(MakeCard(B, W.NameB, 1, EBfkCardKind::Skill, EBfkRarity::Common, EBfkElement::Arcane, EBfkTarget::None, { FBfkEffect(EBfkOp::Draw, 1), FBfkEffect(EBfkOp::Energy, 1), FBfkEffect(EBfkOp::ExhaustSelf, 0) }));
		}
		else if (C == TEXT("shield"))
		{
			AddCard(MakeCard(A, W.NameA, 1, EBfkCardKind::Attack, EBfkRarity::Common, EBfkElement::Iron, EBfkTarget::EnemyFront, { FBfkEffect::Dmg(4), FBfkEffect(EBfkOp::Push, 2) }));
			AddCard(MakeCard(B, W.NameB, 1, EBfkCardKind::Skill, EBfkRarity::Common, EBfkElement::Iron, EBfkTarget::None, { FBfkEffect::Blk(9) }));
		}
		else if (C == TEXT("tome"))
		{
			AddCard(MakeCard(A, W.NameA, 1, EBfkCardKind::Skill, EBfkRarity::Common, EBfkElement::Arcane, EBfkTarget::None, { FBfkEffect(EBfkOp::Draw, 2) }));
			AddCard(MakeCard(B, W.NameB, 2, EBfkCardKind::Skill, EBfkRarity::Common, EBfkElement::Arcane, EBfkTarget::None, { FBfkEffect(EBfkOp::Draw, 2), FBfkEffect(EBfkOp::Energy, 1) }));
		}
		else if (C == TEXT("instrument"))
		{
			AddCard(MakeCard(A, W.NameA, 1, EBfkCardKind::Skill, EBfkRarity::Common, EBfkElement::Neutral, EBfkTarget::None, { FBfkEffect{EBfkOp::StatusSelfSide, 1, EBfkStatus::Rally} }));
			AddCard(MakeCard(B, W.NameB, 1, EBfkCardKind::Skill, EBfkRarity::Common, EBfkElement::Neutral, EBfkTarget::None, { FBfkEffect{EBfkOp::HealAll, 3} }));
		}
		else if (C == TEXT("scythe"))
		{
			AddCard(MakeCard(A, W.NameA, 2, EBfkCardKind::Attack, EBfkRarity::Common, EBfkElement::Shadow, EBfkTarget::Lane, { FBfkEffect{EBfkOp::DamageRow, 7} }));
			AddCard(MakeCard(B, W.NameB, 1, EBfkCardKind::Attack, EBfkRarity::Common, EBfkElement::Shadow, EBfkTarget::Enemy, { FBfkEffect::Dmg(6), FBfkEffect{EBfkOp::Heal, 3} }));
		}
		else
		{
			AddCard(MakeCard(A, W.NameA, 1, EBfkCardKind::Attack, EBfkRarity::Common, EBfkElement::Neutral, EBfkTarget::Enemy, { FBfkEffect::Dmg(6) }));
			AddCard(MakeCard(B, W.NameB, 1, EBfkCardKind::Attack, EBfkRarity::Common, EBfkElement::Neutral, EBfkTarget::Enemy, { FBfkEffect::Dmg(4), FBfkEffect(EBfkOp::Push, 1) }));
		}
	}

	// assign weapon granted cards
	for (auto& KV : GWeapons)
	{
		FBfkWeaponDef& W = KV.Value;
		if (W.GrantedCards.Num()) continue;
		const FName A(*FString::Printf(TEXT("wkit-%s-a"), *W.WClass));
		const FName B(*FString::Printf(TEXT("wkit-%s-b"), *W.WClass));
		if (GCards.Contains(A)) W.GrantedCards.Add(A);
		if (W.Rarity == EBfkRarity::Legendary && GCards.Contains(B)) W.GrantedCards.Add(B);
		else if (GCards.Contains(B) && (GetTypeHash(W.Slug) & 1)) W.GrantedCards.Add(B);
	}

	// ---------- give every species without a hand-authored kit its combo kit ----------
	// element -> spell pool for flavor picks
	TMap<EBfkElement, TArray<FName>> SpellPool;
	for (const auto& KV : GCards)
	{
		if (KV.Value.ArtSlug.ToString().StartsWith(TEXT("card_")))
		{
			SpellPool.FindOrAdd(KV.Value.Element).Add(KV.Key);
		}
	}
	for (auto& KV : SpellPool) KV.Value.Sort([](const FName& L, const FName& R){ return L.LexicalLess(R); });

	static const TCHAR* ElemKitName[] = { TEXT("cinder"), TEXT("hoarfrost"), TEXT("briar"), TEXT("galestorm"), TEXT("umbral"), TEXT("ironclad"), TEXT("runeglow"), TEXT("gray") };

	for (FName SpSlug : GSpeciesOrder)
	{
		FBfkSpeciesDef& Sp = GSpecies[SpSlug];
		if (Sp.SignatureCards.Num()) continue; // heroes/bosses done
		const int32 Ei = (int32)Sp.Element;
		const FString ArchName = Bfk::ArchetypeName(Sp.Archetype).ToLower();
		const FName A(*FString::Printf(TEXT("kit-%s-%s-a"), ElemKitName[Ei], *ArchName));
		const FName B(*FString::Printf(TEXT("kit-%s-%s-b"), ElemKitName[Ei], *ArchName));
		const FName C(*FString::Printf(TEXT("kit-%s-%s-c"), ElemKitName[Ei], *ArchName));
		Sp.SignatureCards = { A, B, C };

		// flavor spell(s) from the element's card-art pool, deterministic per species
		if (const TArray<FName>* Pool = SpellPool.Find(Sp.Element))
		{
			if (Pool->Num())
			{
				const uint32 H = GetTypeHash(Sp.Slug);
				Sp.SignatureCards.Add((*Pool)[H % Pool->Num()]);
				if (Sp.Quality >= 3 && Pool->Num() > 1)
				{
					Sp.SignatureCards.AddUnique((*Pool)[(H / 7) % Pool->Num()]);
				}
			}
		}
	}

	// ---------- special relics (now that sprites are known) ----------
	auto Special = [](FName Slug, const FString& Display, const FString& Desc, EBfkRarity R,
	                  EBfkRelicHook Hook, std::initializer_list<FBfkEffect> Fx,
	                  int32 Hp, int32 Pw, int32 En)
	{
		FName Sprite;
		if (const FBfkRelicDef* Old = GRelics.Find(Slug)) Sprite = Old->SpriteSlug;
		FBfkRelicDef D;
		D.Slug = Slug; D.Display = Display; D.Desc = Desc; D.Rarity = R; D.SpriteSlug = Sprite;
		D.Hook = Hook; D.HpMod = Hp; D.PowerMod = Pw; D.EnergyMod = En;
		for (const FBfkEffect& F : Fx) D.HookEffects.Add(F);
		FBfkContent::AddRelic(D);
	};
	// Power at a price — equipped to ONE teammate.
	int32 Patched = 0;
	TArray<FName> RelicNames;
	GRelics.GenerateKeyArray(RelicNames);
	RelicNames.Sort([](const FName& L, const FName& R){ return L.LexicalLess(R); });
	auto FirstRelicMatching = [&RelicNames](const TCHAR* Pat) -> FName
	{
		for (FName N : RelicNames) if (N.ToString().Contains(Pat)) return N;
		return NAME_None;
	};
	struct FSpec { const TCHAR* Pat; const TCHAR* Name; const TCHAR* Desc; EBfkRarity R; EBfkRelicHook H; FBfkEffect Fx; int32 Hp; int32 Pw; int32 En; };
	const FSpec Specs[] = {
		{ TEXT("pearl"),   TEXT("Pearl of the Drowned Court"), TEXT("At each turn start, gain 3 Ward — but max HP -8. The sea keeps its cut."), EBfkRarity::Rare, EBfkRelicHook::OnTurnStart, FBfkEffect::St(EBfkStatus::Ward, 3), -8, 0, 0 },
		{ TEXT("skull"),   TEXT("Grinning Toll"), TEXT("+3 Power. Each battle start, take 4 damage. It laughs when you bleed."), EBfkRarity::Rare, EBfkRelicHook::OnBattleStart, FBfkEffect(EBfkOp::DamageSelf, 4), 0, 3, 0 },
		{ TEXT("lantern"), TEXT("Forgekeeper's Lantern"), TEXT("The squad gains +1 energy — holder's max HP -10. The flame feeds on someone."), EBfkRarity::Legendary, EBfkRelicHook::Passive, FBfkEffect(EBfkOp::Damage, 0), -10, 0, 1 },
		{ TEXT("eye"),     TEXT("Unblinking Eye"), TEXT("Draw +1 at battle start; on kill, draw 1. It sees, and you owe it: -5 max HP."), EBfkRarity::Rare, EBfkRelicHook::OnKill, FBfkEffect(EBfkOp::Draw, 1), -5, 0, 0 },
		{ TEXT("anchor"),  TEXT("Barnacled Anchor"), TEXT("On shove: your shoves gain brutal weight (+2 Power), but holder -6 max HP."), EBfkRarity::Rare, EBfkRelicHook::OnShove, FBfkEffect::St(EBfkStatus::Rally, 1), -6, 2, 0 },
		{ TEXT("heart"),   TEXT("Ember-Heart"), TEXT("+12 max HP. When wounded below half, ignite: gain 2 Rally."), EBfkRarity::Rare, EBfkRelicHook::OnHpBelowHalf, FBfkEffect::St(EBfkStatus::Rally, 2), 12, 0, 0 },
	};
	for (const FSpec& S : Specs)
	{
		const FName Found = FirstRelicMatching(S.Pat);
		if (!Found.IsNone())
		{
			Special(Found, S.Name, S.Desc, S.R, S.H, { S.Fx }, S.Hp, S.Pw, S.En);
			++Patched;
		}
	}
}

// ============================================================== assembly helpers

TArray<FName> FBfkContent::DeckFor(const FBfkFighterSpec& Spec)
{
	EnsureInit();
	TArray<FName> Out;
	if (const FBfkSpeciesDef* Sp = Species(Spec.Species))
	{
		Out.Append(Sp->SignatureCards);
	}
	if (const FBfkWeaponDef* W = Weapon(Spec.Weapon))
	{
		Out.Append(W->GrantedCards);
	}
	if (!Spec.InheritedCard.IsNone() && Card(Spec.InheritedCard))
	{
		Out.Add(Spec.InheritedCard);
	}
	return Out;
}

TArray<FName> FBfkContent::CardRewardChoices(const TArray<FName>& SquadSpecies, FRandomStream& Rng, int32 Count, bool bElite)
{
	EnsureInit();
	TArray<FName> Pool;
	TSet<EBfkElement> SquadElems;
	for (FName S : SquadSpecies)
	{
		if (const FBfkSpeciesDef* Sp = Species(S))
		{
			Pool.Append(Sp->SignatureCards);
			SquadElems.Add(Sp->Element);
		}
	}
	// spells of squad elements + neutral utility
	for (const auto& KV : GCards)
	{
		const bool bSpell = KV.Value.ArtSlug.ToString().StartsWith(TEXT("card_"));
		const bool bNeutralUtil = KV.Value.Rarity != EBfkRarity::Starter && !bSpell
			&& !KV.Key.ToString().StartsWith(TEXT("kit-")) && !KV.Key.ToString().StartsWith(TEXT("wkit-"))
			&& KV.Value.Element == EBfkElement::Neutral;
		if ((bSpell && (SquadElems.Contains(KV.Value.Element) || KV.Value.Element == EBfkElement::Neutral)) || bNeutralUtil)
		{
			Pool.Add(KV.Key);
		}
	}
	Pool.Sort([](const FName& L, const FName& R){ return L.LexicalLess(R); });

	TArray<FName> Out;
	int32 Guard = 0;
	while (Out.Num() < Count && ++Guard < 500 && Pool.Num())
	{
		const FName Pick = Pool[Rng.RandRange(0, Pool.Num() - 1)];
		const FBfkCardDef* D = Card(Pick);
		if (!D || Out.Contains(Pick)) continue;
		// rarity gate
		const int32 Roll = Rng.RandRange(1, 100);
		const int32 EliteShift = bElite ? 15 : 0;
		bool bOk = false;
		switch (D->Rarity)
		{
		case EBfkRarity::Common:    bOk = true; break;
		case EBfkRarity::Uncommon:  bOk = Roll <= 55 + EliteShift; break;
		case EBfkRarity::Rare:      bOk = Roll <= 22 + EliteShift; break;
		case EBfkRarity::Legendary: bOk = Roll <= 6 + EliteShift; break;
		default: bOk = false; break;
		}
		if (bOk) Out.Add(Pick);
	}
	return Out;
}

TArray<FName> FBfkContent::RollEncounter(int32 Act, bool bElite, FRandomStream& Rng)
{
	EnsureInit();
	TArray<FName> Minions, Elites;
	for (FName S : GSpeciesOrder)
	{
		const FBfkSpeciesDef& Sp = GSpecies[S];
		if (!Sp.bEnemyOnly || Sp.bBoss) continue;
		if (Sp.bMinion) Minions.Add(S);
		else Elites.Add(S);
	}
	TArray<FName> Out;
	auto PickFrom = [&Rng](const TArray<FName>& Arr) { return Arr[Rng.RandRange(0, Arr.Num() - 1)]; };
	if (bElite)
	{
		Out.Add(PickFrom(Elites));
		if (Act >= 4) Out.Add(PickFrom(Elites)); else Out.Add(PickFrom(Minions));
		Out.Add(PickFrom(Minions));
	}
	else
	{
		Out.Add(PickFrom(Minions));
		Out.Add(PickFrom(Minions));
		if (Act >= 3) Out.Add(PickFrom(Elites)); else Out.Add(PickFrom(Minions));
	}
	return Out;
}

FName FBfkContent::BossFor(int32 Act)
{
	switch (Act)
	{
	case 1: return TEXT("rot-shepherd");
	case 2: return TEXT("ghostwake");
	case 3: return TEXT("facet-queen");
	default: return TEXT("tidebound-king");
	}
}

TArray<FName> FBfkContent::BossMinions(FName BossSlug)
{
	EnsureInit();
	if (BossSlug == TEXT("rot-shepherd")) return { GAliasSporeling, GAliasSporeling };
	if (BossSlug == TEXT("ghostwake")) return { GAliasDeckhand, GAliasDeckhand };
	// later bosses bring a random minion pair
	TArray<FName> Minions;
	for (FName S : GSpeciesOrder)
	{
		const FBfkSpeciesDef& Sp = GSpecies[S];
		if (Sp.bEnemyOnly && Sp.bMinion) Minions.Add(S);
	}
	return { Minions[GetTypeHash(BossSlug) % Minions.Num()], Minions[(GetTypeHash(BossSlug) / 3) % Minions.Num()] };
}

FString FBfkContent::ActName(int32 Act)
{
	switch (Act)
	{
	case 1: return TEXT("Blackroot Wilds");
	case 2: return TEXT("The Drowned Wharf");
	case 3: return TEXT("Crystal Hollow");
	default: return TEXT("The Shadow Isle");
	}
}

FString FBfkContent::ActBackdrop(int32 Act)
{
	switch (Act)
	{
	case 1: return TEXT("bg_darkforest");
	case 2: return TEXT("bg_darkdocks");
	case 3: return TEXT("bg_crystalcove");
	default: return TEXT("bg_shadowisle");
	}
}

EBfkWeather FBfkContent::RollWeather(int32 Act, FRandomStream& Rng)
{
	const int32 R = Rng.RandRange(1, 100);
	switch (Act)
	{
	case 1: return R <= 55 ? EBfkWeather::Rain : EBfkWeather::Gloom;
	case 2: return R <= 70 ? EBfkWeather::Rain : EBfkWeather::Gloom;
	case 3: return R <= 70 ? EBfkWeather::Snow : EBfkWeather::Gloom;
	default: return R <= 70 ? EBfkWeather::Ashfall : (R <= 85 ? EBfkWeather::Rain : EBfkWeather::Gloom);
	}
}

TArray<FName> FBfkContent::RelicPool(FRandomStream& Rng, int32 Count, const TSet<FName>& Exclude)
{
	EnsureInit();
	TArray<FName> All;
	for (const auto& KV : GRelics) if (!Exclude.Contains(KV.Key)) All.Add(KV.Key);
	All.Sort([](const FName& L, const FName& R){ return L.LexicalLess(R); });
	TArray<FName> Out;
	while (Out.Num() < Count && All.Num())
	{
		const int32 I = Rng.RandRange(0, All.Num() - 1);
		Out.Add(All[I]);
		All.RemoveAt(I);
	}
	return Out;
}

TArray<FName> FBfkContent::WeaponPool(FRandomStream& Rng, int32 Count, int32 Act, const TSet<FName>& Exclude)
{
	EnsureInit();
	TArray<FName> All;
	for (const auto& KV : GWeapons)
	{
		if (Exclude.Contains(KV.Key)) continue;
		if (KV.Value.Rarity == EBfkRarity::Legendary && Act < 2) continue;
		All.Add(KV.Key);
	}
	All.Sort([](const FName& L, const FName& R){ return L.LexicalLess(R); });
	TArray<FName> Out;
	while (Out.Num() < Count && All.Num())
	{
		const int32 I = Rng.RandRange(0, All.Num() - 1);
		const FBfkWeaponDef* W = Weapon(All[I]);
		// legendaries are rare finds
		if (W->Rarity == EBfkRarity::Legendary && Rng.RandRange(1, 100) > 20 + Act * 8)
		{
			All.RemoveAt(I);
			continue;
		}
		Out.Add(All[I]);
		All.RemoveAt(I);
	}
	return Out;
}

TArray<FName> FBfkContent::StarterSquad()
{
	EnsureInit();
	TArray<FName> Out;
	Out.Add(TEXT("captain-blacktide"));
	if (GSpecies.Contains(TEXT("wolf-squire"))) Out.Add(TEXT("wolf-squire"));
	// third member: a caster beast that isn't frost, for elemental variety
	for (FName S : GSpeciesOrder)
	{
		if (Out.Num() >= 3) break;
		const FBfkSpeciesDef& Sp = GSpecies[S];
		if (Sp.bBeast && !Sp.bEnemyOnly && Sp.Archetype == EBfkArchetype::Caster
			&& Sp.Element != EBfkElement::Frost && Sp.Element != EBfkElement::Iron && !Out.Contains(S))
		{
			Out.Add(S);
		}
	}
	while (Out.Num() < 3)
	{
		for (FName S : GSpeciesOrder)
		{
			const FBfkSpeciesDef& Sp = GSpecies[S];
			if (Sp.bBeast && !Sp.bEnemyOnly && !Out.Contains(S)) { Out.Add(S); break; }
		}
	}
	return Out;
}

TArray<FName> FBfkContent::CapturableSpecies(int32 Act, FRandomStream& Rng, int32 Count)
{
	EnsureInit();
	TArray<FName> All;
	for (FName S : GSpeciesOrder)
	{
		const FBfkSpeciesDef& Sp = GSpecies[S];
		if (Sp.bBeast && !Sp.bEnemyOnly && !Sp.bHumanoid && Sp.Quality <= (Act >= 3 ? 3 : 2)) All.Add(S);
	}
	TArray<FName> Out;
	while (Out.Num() < Count && All.Num())
	{
		const int32 I = Rng.RandRange(0, All.Num() - 1);
		Out.Add(All[I]);
		All.RemoveAt(I);
	}
	return Out;
}

FName FBfkContent::BreedResult(FName ParentA, FName ParentB, FRandomStream& Rng)
{
	EnsureInit();
	const FBfkSpeciesDef* A = Species(ParentA);
	const FBfkSpeciesDef* B = Species(ParentB);
	if (!A || !B) return NAME_None;

	const int32 MaxQ = FMath::Max(A->Quality, B->Quality) + (Rng.RandRange(1, 100) <= 12 ? 1 : 0);
	TArray<FName> Candidates;
	for (FName S : GSpeciesOrder)
	{
		const FBfkSpeciesDef& Sp = GSpecies[S];
		if (!Sp.bBeast || Sp.bEnemyOnly || Sp.bHumanoid || S == ParentA || S == ParentB) continue;
		if (Sp.Quality > MaxQ) continue;
		const bool bElemMatch = Sp.Element == A->Element || Sp.Element == B->Element;
		const bool bArchMatch = Sp.Archetype == A->Archetype || Sp.Archetype == B->Archetype;
		if (bElemMatch && bArchMatch) { Candidates.Add(S); Candidates.Add(S); } // weight double
		else if (bElemMatch || bArchMatch) Candidates.Add(S);
	}
	if (!Candidates.Num()) return NAME_None;
	return Candidates[Rng.RandRange(0, Candidates.Num() - 1)];
}
