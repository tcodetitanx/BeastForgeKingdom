# Session handoff — Claude build session, 2026-07-01

## Update: home-PC session, 2026-07-01 (evening)

Project cloned to Daniel's home PC (D:\UEProjects\BeastForgeKingdom, engine at
D:\UE_5.8 — a DIFFERENT UE 5.8 build than the original machine). Daniel
playtested and drove an art/UX overhaul:

- **Isometric battle board**: 3x4 grid now renders as 300x150 diamond tiles
  (`CellCenter`), fixed angle, painter's-order ZOrder by row+col, procedural
  `ui_iso_tile` texture (Tools/make_iso_tile.py). Removed the ghostfire lane
  divider ("big blue thing"). Drag-to-pan + wheel-zoom (0.8-2.4x, cursor-
  centric) via render transform on BoardLayer+Particles; shake composes.
- **Defringe**: Tools/defringe.py strips the white sheet fringe from all 1114
  sprites (relative-brightness test; unconditional 1px shave + 3 passes for
  character prefixes). Verified clean at 3x zoom.
- **Squish fix**: creature sprites are ~0.6 aspect; meta screens were stretching
  them into squares. New `BfkUi::SpriteFit` used in vault/squad/library/
  breeding/map (8 sites).
- **2x status/hazard icons** with hover tooltips (StatusDesc/HazardDesc).
- **Pause menu (Esc or top-right chip) + full Codex** (keywords, statuses,
  hazards, weather, capture, breeding, currencies) — built into UBfkScreen,
  overlay constructed at screen build time (see engine gotcha below).
- **ENGINE GOTCHA (this machine)**: FSlateBrush DrawAs=Image with no resource
  object renders NOTHING. SolidBrush now always uses RoundedBox. This also
  resurrected the AddBackdrop darken shade + menu cover panels + pvp curtain,
  which had been silently invisible on this engine.
- **Demo driver hardened**: design->screen mapping now goes through the active
  screen's cached geometry (window rect lied when the desktop moved windows);
  rclick routes to `UBfkBattleScreen::CancelTargeting`; clicks respect the
  modal pause overlay. demo_battle.txt re-coordinated for the iso board +
  pause/codex steps. Hand cards now PARK at their rest position (transform
  resets can no longer drop the hand offscreen).
- All 4 automation suites green; full demo tour runs with zero warnings.

This document is the memory of the Claude session that built BeastForge Kingdom
from an empty UE 5.8 project in one sitting. If you are a Claude instance (or a
human) picking this up on another machine: read this top to bottom, then
`CLAUDE.md` for commands, then `Docs/GDD.md` + `Docs/LORE.md` for design intent.

## The original brief (condensed)

Daniel (GitHub `tcodetitanx`) asked for a complete, polished 3v3 deck-builder
creature-collector dark-fantasy roguelite mixing Slay the Spire + Pirates
Outlaws + creature collection, with: squad-composition-driven decks (both
sides), per-teammate relics with costs, weapons as a mechanic, breeding +
capture as permanent progression, unlockable weapons/relics/spells, a 4-act
campaign with bosses and novel mechanics, local battle + hotseat PvP modes,
menus/settings/story/lore, procedural animation (squash/flash/splines) using
the particle sprites, dark & dreary mood with rain/snow, and Into-the-Breach
positional play. Sprites face LEFT (flip allies). Audio from a Unity pack
(strip .meta). Full autonomy granted; publish to GitHub when done.

## What exists and works (verified via scripted playtests + 4 test suites)

- **Full game loop**: intro (4-page typewriter lore) -> main menu (live widgets
  composited over the painted `bg_main_menu` mockup) -> squad muster (pick 3 +
  equip owned weapons/relics) -> act map (StS node web) -> battles (3x4 board,
  lane attacks, shoves/collisions, hazards, telegraphed intents, weather
  effects, capture via auto-added Soul Snare token card) -> rewards (gold,
  squad-scoped card choices, relic/weapon drops with equip-to-whom buttons,
  egg hatching, milestones) -> shops/events/forge/rest/breeding dens -> boss ->
  next act -> victory screen. Death = meta-progress kept, run ends.
- **Bosses** (coded hooks in `BfkBattle::BossTurnStart`): rot-shepherd
  (spreading spores + sporeling summons), ghostwake (capsizes your columns
  every 3 turns + deckhands), facet-queen (reflects first attack each turn,
  crystal shards), tidebound-king (marks a lane with Floodmark, floods it next
  turn: damage + upward shove cascade; speech barks).
- **Meta**: vault (rename via EditableTextBox), breeding (element-or-archetype
  compatibility, emberglass cost, egg hatches after N victories, stat/card
  inheritance, generations), enemy library (silhouettes until seen), 10
  milestones, settings (volumes, weather intensity, shake, fast anims, window
  mode, 2-click profile reset), local battle setup (random foes + weather),
  PvP hotseat (two squad picks -> curtain between turns).
- **Content**: generated from labeled sprite manifests (`Tools/labels/*.json`,
  produced by vision-labeling 1119 auto-extracted sprites). 285 ally species,
  77 enemies (32 minions, ~40 elites, 4 bosses mapped from best-fit sprites),
  128 weapons, 80 relics (6 hand-authored "power at a price" specials override
  generated fallbacks by slug-keyword), 128 art-derived spell cards + hero/boss
  kits + 48 element-x-archetype template kits + weapon-class cards.
- **Presentation**: procedural Slate weather (rain streaks/snow/ash + gloom
  pulse), sprite-particle bursts/flourishes/floaties, tween library (lunge,
  squash-hit, death dissolve, deal-in), bezier projectile streaks, screen
  shake, boss speech box, banners. SFX: 61 curated wavs mapped in
  `Tools/import_assets.py` (slug -> pack file), hooked to battle events by
  type + element. Fonts: Cardo (body), Pirata One (display), OFL, in
  `Content/BFK/Fonts` loaded by file path.

## Session history (what happened, in order)

1. Explored project; found sheets partially auto-imported as uassets; exported
   all 42 back to PNG headless (`Tools/export_textures.py`).
2. Built `Tools/sprite_pipeline.py`: border flood-fill white removal with
   near-white threshold 232, enclosed-pocket detection (pure-white area >= 60px
   inside sprites -> background), anti-aliased edge un-blending, connected
   components with small-fragment adoption, reading-order sort, numbered
   contact sheets. 1119 sprites from 38 sheets.
3. Fanned out 5 vision agents to label everything -> `Tools/labels/*.json`.
4. Wrote GDD + lore; configured engine MCP server + wrote `Tools/mcp.ps1`.
5. Implemented C++ core (types -> battle sim -> content registries ->
   generator -> run -> meta -> game framework -> widget lib -> 20 screens).
6. Imported 1114 textures + 61 sounds; created Boot map; wrote 4 automation
   suites; all green.
7. Discovered the Windows session was LOCKED -> desktop screenshots/input
   impossible -> built `-BfkDemo` in-engine driver (script of clicks/shots)
   with console cmds `Bfk.NewRun`/`Bfk.Enter` for deterministic tours.
8. Iterated via screenshots; fixed: FX layers swallowing clicks (HitTestInvisible
   in ctor), instant-victory bug (all-minion encounters died to the "minions
   don't hold a side" rule -> replaced with bOriginal reinforcement flag),
   intent text overlapping the row above, layout-vs-render-bounds hit testing,
   menu double-render vs painted mockup (opaque covers + real widgets),
   puddle icon, starter squad element diversity (captain/wolf/owl-oracle).
9. Pruned audio (625 -> 61 wavs, .meta stripped), git init with UE .gitignore
   (+ keys/), created repo via API, pushed. Added screenshot gallery.

## Known issues / rough edges (honest list)

- Enemy AI targeting for MoveSelf/AllySlot intents picks player-side cells and
  wastes the action (cosmetically fine, reads as "hesitates").
- `t_menu` cover panels are tuned to the painted mockup at 1920x1080 design
  space; other aspect ratios letterbox via UMG DPI scaling (ShortestSide).
- Cards without dedicated art use cardback + element wash + text plate; looks
  intentional but a nicer generated frame would help.
- Hand cards >8 overlap tightly; no drag-to-play (click card, click target).
- No music track (pack is SFX-only); ambience slot exists (`sfx_rumble` etc.).
- PvP shares one saved vault for both players (by design for hotseat).
- Balance is first-pass: sims show mixed win/loss but no depth tuning.
- `Saved/` is untracked: fresh clone = fresh profile (bSeenIntro false -> intro
  plays; starter vault: captain-blacktide, wolf-squire, owl-oracle + 2 common
  weapons).

## Backlog / good next steps

1. Card upgrade preview + upgraded-copy handling per-instance (currently
   `Run.UpgradedCards` marks all copies of a slug).
2. Deck viewer overlay in battle (draw/discard pile inspection).
3. Elite/boss intro splash using the big elite sprites; capture flourish.
4. Ambient loop per act (license a track or synthesize); volume routing exists.
5. Balance pass: use `BeastForge.Battle.RandomSim` win-rate telemetry per act.
6. Steam-style achievements from milestones; more milestones w/ relic rewards
   (table supports RewardRelic/RewardWeapon already).
7. Drag-to-play cards; hover tooltips for statuses on tokens.
8. More boss mechanics variety (the scaffolding: `BossTurnStart` + speech).

## Environment facts from the original machine

- Engine: UE 5.8 SOURCE BUILD at `D:\UE_5.8` (uproject association changed to
  generic "5.8" for portability — re-associate on clone if needed).
- Python 3.11 + Pillow + numpy + scipy used by Tools/ (only needed to re-run
  the art pipeline, not to build/play).
- The GitHub token lives ONLY in `Content/keys/githubKey.txt` (git-ignored):
  line 1 token, line 3 username. Recreate this file on a new machine to push.
  The token seen in-session had very broad scopes — recommend rotating to a
  repo-scoped token.
- Original working dir: `D:\UEprojects\BeastForgeKingdom`; user email
  daniel@bullaxiom.com; commits authored as tcodetitanx.

## How to "resume being me"

Treat `Docs/GDD.md` as design law and this file as your memory. Daniel's
standing preferences: full autonomy (don't ask, verify yourself via the demo
driver + tests), dark/dreary tone everywhere, high polish bar, keep the keys
folder out of git, delete unused assets before committing. When you change
battle logic, run the 4 automation suites; when you change UI, run a
`-BfkDemo` tour and LOOK at the screenshots before declaring success.
