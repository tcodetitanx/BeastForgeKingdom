# BeastForge Kingdom — Game Design Document

A 3v3 deck-building creature-collector roguelite. Slay the Spire's run structure and
card play, Pirates Outlaws' grim seafaring flavor, Into the Breach's positional
chess, and a creature-collection/breeding meta that persists between runs.
Mood: dark, dreary, rain-slick, lantern-lit.

## 1. Core Loop

RUN (roguelite): pick 3 squad members → travel a node map through an Act →
battles/events/shops/forges → Act boss → next Act (4 Acts) → win or die.
Death is progress: everything you captured, bred, and unlocked stays.

META (permanent): Beast Vault (collection), Breeding Chambers, weapon/relic/spell
unlocks (Forge Milestones), currencies.

## 2. The Squad IS the Deck

- You field exactly 3 units (Forgewarden heroes and/or beasts).
- Each unit contributes its **signature cards** (4-5 each) to a shared battle deck.
- Each unit may hold one **weapon** (adds 2-3 weapon cards + stat mod)
  and one **relic** (warps that unit's abilities — power at a price).
- Card rewards offered after battles are drawn ONLY from the pools of the three
  squad members' species/classes (plus neutral spells). Change the squad,
  change the game. Enemy decks are likewise generated from the enemy trio.
- If a unit dies in battle, its cards are severed (unplayable, dead weight) until revived.

## 3. Battle System — 3v3 on a 3x4 tactical board

Board: 3 rows (lanes) x 4 columns. Columns 0-1 = ally side, 2-3 = enemy side.
Sprites face right for allies (flipped; art faces left), enemies face left (native art).

- **Energy**: 3 per turn (+weapon/relic mods). Draw 5. Discard at end of turn.
- **Lanes matter**: most attacks are LANE attacks — they hit the first enemy in
  the attacker's lane. Back column = safe from melee but many attacks are
  melee-range only (front column). Ranged/spell cards reach anywhere.
- **Movement & shoves**: cards push, pull, and swap units. Shoving an enemy into
  the board edge or another unit = collision damage to both (Into the Breach).
  Shoving an enemy out of its telegraphed attack's lane WASTES its attack.
- **Hazard cells**: burning coals (burn), hoarfrost (chill), void rifts (curse),
  powder kegs (explode when a unit is shoved in). Created by cards, bosses, weather.
- **Intents**: every enemy telegraphs next turn's card and its target lane/cell.
  Positioning play: dodge, shove, body-block, or kill before it fires.
- **Statuses**: Burn (dmg/turn, stacks), Chill (energy/dmg debuff), Poison
  (escalating), Shock (chains to adjacent on hit), Curse (blocks healing),
  Rust (armor melt), Ward (block that persists), Rally (dmg up).
- **Capture**: beat a beast-type enemy while your squad has an empty Soul Snare
  charge → chance to capture it into your Vault (higher when it's low HP and
  the battle used no deaths). This is the collector engine.

## 4. Weather (mechanical mood)

Every battle rolls weather from its region: **Rain** (ember -1 dmg, storm +1,
puddle cells conduct shock), **Snow** (chill accumulates each turn on the exposed
back column, frost cards +1), **Ashfall** (shadow +1, healing -1), **Clear-dark**.
Rendered as full-screen particle layers; always dreary.

## 5. Meta Progression

- **Beast Vault**: every captured/bred beast, permanent. Squads are drafted from it.
- **Breeding Chambers**: pair two beasts → egg (element mix, stat blend, chance to
  inherit a mutated signature card, small rarity-upgrade chance). Eggs hatch after
  N run-victories. Costs Emberglass.
- **Forge Milestones**: achievement-driven unlocks of new weapons, relics, neutral
  spells, and starting heroes (e.g. "win a battle without taking damage → unlock
  Aegis Charm").
- **Currencies**: Gold (in-run only), Soulshards (meta, earned per run, buys
  unlock rerolls/eggs), Emberglass (rare breeding catalyst from bosses/elites).

## 6. Campaign — The Drowned Kingdom of Vhal'Mora

Act 1 **Blackroot Wilds** (darkForest, Rain) — boss: the Rot Shepherd
Act 2 **The Drowned Wharf** (darkDocks, Rain/Storm) — boss: Ghostwake, Reaver of the Deep
Act 3 **Crystal Hollow** (crystalCove, Snow) — boss: the Facet Queen
Act 4 **The Shadow Isle** (shadowIsle, Ashfall) — final boss: the Tidebound King
Extra area backdrops are used by events, shops, breeding dens, forges.

Node map per act (StS style): Battle, Elite, Event, Shop, Forge (upgrade cards /
socket weapons), Breeding Den (wild egg), Rest (heal or hatch), Boss.
Bosses have multi-phase decks, board-wide hazard mechanics, and drop Embers
(guaranteed relic + Emberglass).

## 7. Modes

- **Adventure**: the campaign roguelite.
- **Local Battle**: single battle vs AI — any squad from your Vault, pick enemy trio & weather.
- **PvP (hotseat)**: two players draft squads from the Vault, alternating turns
  on one screen with a hand-off privacy curtain.

## 8. Bosses & Signature Mechanics

- **Rot Shepherd**: summons mushroom minions in your back column; spores hazard
  spreads each turn — mow the lawn or drown in poison.
- **Ghostwake**: capsizes columns — periodically swaps your front and back
  columns; punishes static formations. Summons drowned deckhands.
- **Facet Queen**: reflects the first attack each turn back at the attacker's
  cell; shatters into crystal shards (hazards) at phase breaks.
- **Tidebound King**: the tide rises — every 3 turns the bottom lane floods
  (all units there take damage and are shoved up, collisions cascade). Phase 2:
  steals one of your unit's signature cards into HIS deck.

## 9. Art & Audio

- Sprites: extracted from sheets (SourceArt pipeline), white removed, labeled
  manifest at Tools/labels. Allies flipped horizontally (art faces left).
- Cards: pre-framed card art (cost gem baked in) + code-drawn name/text overlay.
- UI: extracted ornate frames/buttons/orbs; main_menu.png as menu backdrop with
  live widgets over the painted buttons.
- Animation: all procedural — eased UMG transforms (attack lunge, hit squash,
  flash, death dissolve+fall), Bezier spline projectile arcs, particle sprite
  bursts, screen shake, rain/snow layers.
- Audio: Fantasy_Game_16bit pack (wav). UI clicks, hits by weapon class,
  elemental casts, captures, ambience loops. Unused files deleted before ship.

## 10. Technical

- UE 5.8, pure C++ (no Blueprint logic). Single Boot map; everything is UMG
  built in C++ (WidgetTree->ConstructWidget), screens managed by a UI router.
- Deterministic battle model (plain structs + RNG stream) fully separated from
  presentation; presentation consumes a battle event queue -> animations.
- Content registries in C++ map slugs -> soft object paths for textures/sounds.
- SaveGame: profile (vault, unlocks, currencies, settings) + in-progress run.
