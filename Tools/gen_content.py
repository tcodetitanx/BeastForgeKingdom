"""Generate BfkContentGenerated.cpp + import_manifest.json from sprite labels."""
import json, os, re

ROOT = r"D:/UEprojects/BeastForgeKingdom"
LAB = os.path.join(ROOT, "Tools", "labels")
SPRITES = "SourceArt/sprites"

def load(name):
    with open(os.path.join(LAB, name), encoding="utf-8") as fh:
        return json.load(fh)

def cesc(s):
    return s.replace("\\", "\\\\").replace('"', '\\"')

def aname(prefix, slug):
    return prefix + re.sub(r"[^a-z0-9]+", "_", slug.lower()).strip("_")

allies = load("allies.json")
enemies = load("enemies.json")
cards_relics = load("cards_relics.json")
weap_proj = load("weapons_projectiles.json")
ui_part = load("ui_particles.json")

imports = []   # (src_rel, asset_name)
lines = []     # generated C++ statements

def add_import(rel_dir, sheet, file, asset):
    imports.append({"src": f"{SPRITES}/{rel_dir}/{sheet}/{file}", "asset": asset})

# ---------------------------------------------------------------- species

ELEM = {"ember":"Ember","frost":"Frost","verdant":"Verdant","storm":"Storm",
        "shadow":"Shadow","iron":"Iron","arcane":"Arcane","neutral":"Neutral"}
ARCH = {"bruiser":"Bruiser","tank":"Tank","striker":"Striker","caster":"Caster",
        "support":"Support","trickster":"Trickster","boss":"Bruiser"}

BOSS_MAP = {   # label slug -> (new slug, display, act)
    "fungal-plague-troll":   ("rot-shepherd", "The Rot Shepherd", 1),
    "skeleton-pirate-captain": ("ghostwake", "Ghostwake, Reaver of the Deep", 2),
    "violet-shard-revenant": ("facet-queen", "The Facet Queen", 3),
    "anchor-drowned-monk":   ("tidebound-king", "The Tidebound King", 4),
}
BOSS_HP = {1: 80, 2: 105, 3: 130, 4: 160}

def hp_for(arch, q):
    base = {"bruiser":32,"tank":40,"striker":25,"caster":21,"support":24,"trickster":23,"boss":60}[arch]
    return base + (q - 1) * 5

def pow_for(arch, q):
    base = {"bruiser":1,"tank":0,"striker":2,"caster":1,"support":0,"trickster":1,"boss":2}[arch]
    return base + (1 if q >= 3 else 0)

def emit_species(slug, display, desc, element, arch, quality, sprite, flags):
    a = ARCH[arch]
    lines.append(
        f'\tSp(TEXT("{slug}"), TEXT("{cesc(display)}"), TEXT("{cesc(desc)}"), '
        f'EBfkElement::{ELEM[element]}, EBfkArchetype::{a}, {quality}, '
        f'{flags.get("hp", hp_for(arch, quality))}, {flags.get("power", pow_for(arch, quality))}, '
        f'TEXT("{sprite}"), {int(flags.get("beast",1))}, {int(flags.get("humanoid",0))}, '
        f'{int(flags.get("enemy",0))}, {int(flags.get("boss",0))}, {int(flags.get("minion",0))});')

protagonists = []
lines.append("\t// ---- ally species (generated from art labels)")
for e in allies:
    if e["slug"].startswith("junk-"): continue
    grp = e["group"]
    rel = {"creatures":"characters/allies/creatures","humanoids":"characters/allies/humanoids",
           "protagonists":"characters/allies"}[grp]
    prefix = {"creatures":"crt_","humanoids":"hum_","protagonists":"pro_"}[grp]
    asset = aname(prefix, e["slug"])
    add_import(rel, e["sheet"], e["file"], asset)
    flags = {"beast": 1 if grp == "creatures" else 0,
             "humanoid": 0 if grp == "creatures" else 1}
    emit_species(e["slug"], e["display"], e["desc"], e["element"], e["archetype"],
                 e.get("quality",1), asset, flags)
    if grp == "protagonists":
        protagonists.append(e["slug"])

lines.append("\t// ---- enemy species")
minion_slugs = []
sporeling = deckhand = None
for e in enemies:
    if e["slug"].startswith("junk-") or e["slug"].endswith("-ii"): continue  # -ii sheet is a duplicate
    asset = aname("enm_" if e["sheet"] != "minionsOne" else "min_", e["slug"])
    add_import("characters/enemies", e["sheet"], e["file"], asset)
    is_minion = e["sheet"] == "minionsOne"
    slug, display = e["slug"], e["display"]
    q = 3 if e.get("boss") else 1
    flags = {"enemy": 1, "beast": 1, "minion": int(is_minion)}
    arch = e["archetype"] if e["archetype"] in ARCH else "bruiser"
    if slug in BOSS_MAP:
        slug, display, act = BOSS_MAP[slug]
        flags.update({"boss": 1, "beast": 0, "hp": BOSS_HP[act], "power": 2})
        arch = "boss"
    elif e.get("boss"):
        # elite: strong single unit, humanoid-looking ones aren't capturable
        humanoid_words = ("knight","doctor","monk","crone","cutthroat","shaman","captain","hag","reaper","priest","warden","banshee","hierophant","pirate")
        flags["beast"] = 0 if any(w in slug for w in humanoid_words) else 1
        flags["hp"] = hp_for(arch, 3) + 12
        flags["power"] = pow_for(arch, 3)
    if is_minion:
        flags["hp"] = 12
        flags["power"] = 0
        minion_slugs.append(slug)
        if sporeling is None and re.search(r"mush|spore|fung|myco", slug): sporeling = slug
        if deckhand is None and re.search(r"pirate|skelet|drown|deck|sail|ghoul|zomb", slug): deckhand = slug
    emit_species(slug, display, e["desc"], e["element"], arch, q, asset, flags)

lines.append(f'\tGAliasSporeling = TEXT("{sporeling or minion_slugs[0]}");')
lines.append(f'\tGAliasDeckhand = TEXT("{deckhand or minion_slugs[-1]}");')

# ---------------------------------------------------------------- spell cards from card art

def card_effects(e):
    """suggest/desc keywords -> (effects, target, kind, meleeOnly)"""
    s = (e.get("suggest","") + " " + e["desc"]).lower()
    c = max(1, min(6, e.get("cost") or 1))
    fx, tgt, kind = [], "Enemy", "Attack"
    def D(n): fx.append(f"FBfkEffect::Dmg({n})")
    def St(st, n, self_side=False):
        if self_side:
            fx.append(f'FBfkEffect{{EBfkOp::StatusSelfSide, {n}, EBfkStatus::{st}}}')
        else:
            fx.append(f'FBfkEffect::St(EBfkStatus::{st}, {n})')
    has = lambda *ws: any(w in s for w in ws)

    if has("poison","venom","toxic","plague"):
        D(2 + c); St("Poison", 1 + c)
    elif has("burn","fire","flame","ember","lava","magma","inferno"):
        D(3 + 2*c); St("Burn", 1 + c//2)
    elif has("frost","ice","freeze","chill","snow"):
        D(3 + 2*c); St("Chill", 1 + c//2)
    elif has("lightning","shock","thunder","storm bolt","spark"):
        D(2 + 2*c); St("Shock", 2 + c//2)
    elif has("curse","hex","doom","dark ritual"):
        D(2 + 2*c); St("Curse", 1 + c//3)
    elif has("shield","block","ward","barrier","protect","armor"):
        kind, tgt = "Skill", "Ally"
        if c >= 3: fx.append(f"FBfkEffect{{EBfkOp::BlockAll, {3 + 2*c}}}"); tgt = "None"
        else: fx.append(f"FBfkEffect::Blk({5 + 3*c})")
        if has("ward","spirit","ghost"): St("Ward", 2 + c)
    elif has("heal","restore","mend","regener","blessing","cure"):
        kind, tgt = "Skill", "Ally"
        if c >= 3: fx.append(f"FBfkEffect{{EBfkOp::HealAll, {2 + 2*c}}}"); tgt = "None"
        else: fx.append(f"FBfkEffect{{EBfkOp::Heal, {4 + 3*c}}}")
    elif has("rally","roar","battle cry","warhorn","banner","inspire","buff"):
        kind, tgt = "Skill", "None"
        St("Rally", 1 + c//2, self_side=True)
    elif has("draw","scroll","tome","book","insight","scry"):
        kind, tgt = "Skill", "None"
        fx.append(f"FBfkEffect{{EBfkOp::Draw, {1 + c//2}}}")
        if c >= 3: fx.append(f"FBfkEffect{{EBfkOp::Energy, 1}}")
    elif has("push","knock","slam","bash","ram","charge","shove"):
        D(3 + 2*c); fx.append(f"FBfkEffect{{EBfkOp::Push, {1 + c//3}}}")
    elif has("pull","hook","chain","drag","harpoon","anchor"):
        D(2 + 2*c); fx.append(f"FBfkEffect{{EBfkOp::Pull, 1}}")
    elif has("all enem","wave","quake","eruption","nova","tempest","storm","rain of","barrage"):
        tgt = "None"
        fx.append(f"FBfkEffect{{EBfkOp::DamageAll, {2 + 2*c}}}")
    elif has("lane","pierce","spear thrust","javelin","volley","bolt","arrow","shot"):
        tgt = "Lane"
        fx.append(f"FBfkEffect{{EBfkOp::DamageLane, {4 + 3*c}}}")
    elif has("stealth","shadow step","vanish","smoke"):
        kind, tgt = "Skill", "Ally"
        St("Stealth", 1 + c//3); fx.append(f"FBfkEffect::Blk({3 + c})")
    elif has("thorn","spike","bristle","retaliat"):
        kind, tgt = "Skill", "Ally"
        St("Thorns", 2 + c); fx.append(f"FBfkEffect::Blk({2 + c})")
    else:
        D(4 + 3*c)
    return fx, tgt, kind

lines.append("\t// ---- spell cards generated from framed card art")
spell_by_elem = {}
for e in cards_relics:
    if e["kind"] != "card" or e["slug"].startswith("junk-"): continue
    asset = aname("card_", e["slug"])
    add_import("cards", e["sheet"], e["file"], asset)
    fx, tgt, kind = card_effects(e)
    c = max(1, min(6, e.get("cost") or 1))
    rar = "Common" if c <= 1 else ("Uncommon" if c <= 3 else ("Rare" if c <= 5 else "Legendary"))
    lines.append(
        f'\tCd(TEXT("{e["slug"]}"), TEXT("{cesc(e["display"])}"), {c}, '
        f'EBfkCardKind::{kind}, EBfkRarity::{rar}, EBfkElement::{ELEM[e["element"]]}, '
        f'EBfkTarget::{tgt}, TEXT("{asset}"), {{ {", ".join(fx)} }});')
    spell_by_elem.setdefault(e["element"], []).append(e["slug"])

# ---------------------------------------------------------------- weapons

lines.append("\t// ---- weapons")
for e in weap_proj:
    if e["slug"].startswith("junk-"): continue
    if e["group"] == "projectiles":
        asset = aname("prj_", e["slug"])
        add_import("projectiles", e["sheet"], e["file"], asset)
        continue
    tier = "legendary" if "legendary" in e["group"] else "common"
    asset = aname("wpn_", e["slug"])
    add_import(e["group"], e["sheet"], e["file"], asset)
    rar = "Legendary" if tier == "legendary" else "Common"
    hp = 4 if e["wclass"] == "shield" else 0
    pw = 0 if e["wclass"] == "shield" else (2 if tier == "legendary" else 1)
    lines.append(
        f'\tWp(TEXT("{e["slug"]}"), TEXT("{cesc(e["display"])}"), TEXT("{cesc(e["desc"])}"), '
        f'TEXT("{e["wclass"]}"), EBfkElement::{ELEM[e["element"]]}, EBfkRarity::{rar}, '
        f'TEXT("{asset}"), {hp}, {pw});')

# ---------------------------------------------------------------- relics (auto tier; core overrides specials)

lines.append("\t// ---- relics (data-driven fallbacks; specials overridden in core)")
relic_slugs = []
for e in cards_relics:
    if e["kind"] != "relic" or e["slug"].startswith("junk-"): continue
    asset = aname("rel_", e["slug"])
    add_import("relics", e["sheet"], e["file"], asset)
    relic_slugs.append(e["slug"])
    s = (e.get("suggest","") + " " + e["desc"]).lower()
    hp, pw, en, hook, hfx = 0, 0, 0, "Passive", ""
    if   re.search(r"heart|life|blood|vital", s): hp = 8
    elif re.search(r"skull|fang|claw|blade|war|rage", s): pw, hp = 2, -5
    elif re.search(r"pearl|shell|shield|stone|iron|armor", s):
        hook, hfx = "OnTurnStart", "FBfkEffect::St(EBfkStatus::Ward, 2)"
        hp = -3
    elif re.search(r"ember|fire|candle|lantern|torch", s):
        hook, hfx = "OnAttack", "FBfkEffect::St(EBfkStatus::Rally, 1)"
        hp = -4
    elif re.search(r"eye|orb|mirror|lens|moon", s):
        hook, hfx = "OnBattleStart", "FBfkEffect{EBfkOp::Draw, 1}"
    elif re.search(r"poison|venom|thorn|spore", s):
        hook, hfx = "OnDamaged", "FBfkEffect::St(EBfkStatus::Thorns, 1)"
    elif re.search(r"feather|wing|boot|swift", s):
        hook, hfx = "OnKill", "FBfkEffect{EBfkOp::Energy, 1}"
    else: hp = 5
    fxs = f"{{ {hfx} }}" if hfx else "{}"
    lines.append(
        f'\tRl(TEXT("{e["slug"]}"), TEXT("{cesc(e["display"])}"), TEXT("{cesc(e["desc"])}"), '
        f'EBfkRarity::Uncommon, TEXT("{asset}"), EBfkRelicHook::{hook}, {fxs}, {hp}, {pw}, {en});')

# ---------------------------------------------------------------- ui / particles / backgrounds

for e in ui_part:
    if e["slug"].startswith("junk-"): continue
    grp = "ui" if e["kind"] == "ui" else "particles"
    add_import(grp, e["sheet"], e["file"], aname("ui_" if grp == "ui" else "par_", e["slug"]))

EXPORTED = "SourceArt/exported"
bg_names = ["crystalCove","darkDocks","darkForest","shadowIsle"]
for i in range(1, 10):
    pass  # unnamed areas handled below
extra_areas = ["ChatGPT_Image_Jul_1__2026__04_37_00_PM__1_","ChatGPT_Image_Jul_1__2026__04_37_01_PM__2_",
"ChatGPT_Image_Jul_1__2026__04_37_01_PM__3_","ChatGPT_Image_Jul_1__2026__04_37_01_PM__4_",
"ChatGPT_Image_Jul_1__2026__04_37_01_PM__5_","ChatGPT_Image_Jul_1__2026__04_37_02_PM__6_",
"ChatGPT_Image_Jul_1__2026__04_37_03_PM__7_","ChatGPT_Image_Jul_1__2026__04_37_03_PM__8_",
"ChatGPT_Image_Jul_1__2026__04_37_03_PM__9_"]
for n in bg_names:
    imports.append({"src": f"{EXPORTED}/areas/{n}.png", "asset": "bg_" + n.lower()})
for i, n in enumerate(extra_areas, 1):
    imports.append({"src": f"{EXPORTED}/areas/{n}.png", "asset": f"bg_area{i}"})
imports.append({"src": f"{EXPORTED}/main_menu/main_menu.png", "asset": "bg_main_menu"})

# procedurally generated ui textures (Tools/make_iso_tile.py, make_hex_tile.py)
imports.append({"src": "SourceArt/generated/ui_iso_tile.png", "asset": "ui_iso_tile"})
imports.append({"src": "SourceArt/generated/ui_hex_tile.png", "asset": "ui_hex_tile"})

# ---------------------------------------------------------------- write outputs

cpp = f"""// GENERATED by Tools/gen_content.py — do not edit by hand.
#include "Core/BfkContent.h"

FName GAliasSporeling;
FName GAliasDeckhand;

namespace
{{
void Sp(const TCHAR* Slug, const TCHAR* Display, const TCHAR* Desc, EBfkElement E,
        EBfkArchetype A, int32 Q, int32 Hp, int32 Power, const TCHAR* Sprite,
        int32 bBeast, int32 bHumanoid, int32 bEnemy, int32 bBoss, int32 bMinion)
{{
	FBfkSpeciesDef S;
	S.Slug = Slug; S.Display = Display; S.Desc = Desc; S.Element = E; S.Archetype = A;
	S.Quality = Q; S.MaxHp = Hp; S.Power = Power; S.SpriteSlug = Sprite;
	S.bBeast = bBeast != 0; S.bHumanoid = bHumanoid != 0; S.bEnemyOnly = bEnemy != 0;
	S.bBoss = bBoss != 0; S.bMinion = bMinion != 0;
	FBfkContent::AddSpecies(S);
}}

void Cd(const TCHAR* Slug, const TCHAR* Display, int32 Cost, EBfkCardKind K, EBfkRarity R,
        EBfkElement E, EBfkTarget T, const TCHAR* Art, std::initializer_list<FBfkEffect> Fx)
{{
	FBfkCardDef C;
	C.Slug = Slug; C.Display = Display; C.Cost = Cost; C.Kind = K; C.Rarity = R;
	C.Element = E; C.Target = T; C.ArtSlug = Art;
	for (const FBfkEffect& F : Fx) C.Effects.Add(F);
	FBfkContent::AddCard(C);
}}

void Wp(const TCHAR* Slug, const TCHAR* Display, const TCHAR* Desc, const TCHAR* WClass,
        EBfkElement E, EBfkRarity R, const TCHAR* Sprite, int32 Hp, int32 Pw)
{{
	FBfkWeaponDef W;
	W.Slug = Slug; W.Display = Display; W.Desc = Desc; W.WClass = WClass; W.Element = E;
	W.Rarity = R; W.SpriteSlug = Sprite; W.HpMod = Hp; W.PowerMod = Pw;
	FBfkContent::AddWeapon(W);
}}

void Rl(const TCHAR* Slug, const TCHAR* Display, const TCHAR* Desc, EBfkRarity R,
        const TCHAR* Sprite, EBfkRelicHook Hook, std::initializer_list<FBfkEffect> Fx,
        int32 Hp, int32 Pw, int32 En)
{{
	FBfkRelicDef D;
	D.Slug = Slug; D.Display = Display; D.Desc = Desc; D.Rarity = R; D.SpriteSlug = Sprite;
	D.Hook = Hook; D.HpMod = Hp; D.PowerMod = Pw; D.EnergyMod = En;
	for (const FBfkEffect& F : Fx) D.HookEffects.Add(F);
	FBfkContent::AddRelic(D);
}}
}} // namespace

void FBfkContent::InitGenerated()
{{
{chr(10).join(lines)}
}}
"""

out_cpp = os.path.join(ROOT, "Source", "BeastForgeKingdom", "Core", "BfkContentGenerated.cpp")
with open(out_cpp, "w", encoding="utf-8") as fh:
    fh.write(cpp)

with open(os.path.join(ROOT, "Tools", "import_manifest.json"), "w", encoding="utf-8") as fh:
    json.dump(imports, fh, indent=0)

print("species allies:", sum(1 for e in allies if not e['slug'].startswith('junk-')))
print("protagonists:", protagonists)
print("sporeling:", sporeling, "| deckhand:", deckhand)
print("minions:", len(minion_slugs))
print("imports:", len(imports))
print("cpp bytes:", len(cpp))
