# BeastForge Kingdom — Claude context

3v3 deck-building creature-collector dark-fantasy roguelite, UE 5.8, 100% C++ (no
Blueprint logic). Built autonomously by a prior Claude session on 2026-07-01.
**Read `Docs/SESSION_HANDOFF.md` first — it is the full state handoff from that
session (what was built, why, known issues, and the backlog).**

## Setup on a fresh machine

1. UE 5.8 required. The uproject's engine association is the custom name
   `BFK-5.8` so the SAME uproject opens on every machine with a source build.
   ONE TIME per machine, register your engine path under that name (else you get
   a "made with a different version of Unreal" error on open):
   `New-ItemProperty -Path "HKCU:\SOFTWARE\Epic Games\Unreal Engine\Builds" -Name "BFK-5.8" -Value "<your UE5.8 root>" -PropertyType String -Force`
   Then, so a local open never rewrites the committed value to your machine's
   GUID: `git update-index --skip-worktree BeastForgeKingdom.uproject`
   (Do NOT run `UnrealVersionSelector /projectfiles` on the .uproject — it prunes
   the custom name and rewrites the association. Use `Tools\build.ps1` /
   generate VS files from the editor instead.)
2. Build: `Tools\build.ps1 -Engine <UE root>` (or set `$env:UE_ROOT`; default
   `D:\UE_5.8`). Or generate VS files and build `BeastForgeKingdomEditor`.
3. Open `BeastForgeKingdom.uproject`, press Play (boot map `/Game/BFK/Maps/Boot`
   is the default; everything is UI). Or launch standalone:
   `UnrealEditor.exe <uproject> -game -windowed -resx=1600 -resy=900`
4. Pushing to GitHub needs `Content/keys/githubKey.txt` (git-ignored; line 1 =
   token, line 3 = username `tcodetitanx`). Ask Daniel for the token.

## Commands

- Build: `Tools\build.ps1` (editor) / `-Game` (game target)
- Tests: `UnrealEditor-Cmd.exe <uproject> -ExecCmds="Automation RunTests BeastForge; Quit" -unattended -nullrhi`
  (4 suites: Content.Sanity, Battle.RandomSim, Run.MapGen, Meta.Breeding — all must stay green)
- Headless editor python: `Tools\uepy.ps1 -Script <abs path .py>`
- Scripted playtest: launch game with `-BfkDemo=<script.txt>` — see
  `Docs/AUTOMATION.md` (wait/click/shot/exec/quit; 1920x1080 design coords;
  console cmds `Bfk.NewRun <seed>`, `Bfk.Enter <nodetype>`)

## Architecture (Source/BeastForgeKingdom)

- `Core/BfkTypes.h` — enums/structs (elements, statuses, hazards, cards, effects)
- `Core/BfkContentCore.cpp` — hand-authored kits (8 heroes, 4 bosses, neutral
  cards, element x archetype template kits, weapon-class cards, special relics),
  encounter/reward/breeding tables
- `Core/BfkContentGenerated.cpp` — **GENERATED, do not hand-edit.** Regenerate:
  `python Tools/gen_content.py` (reads `Tools/labels/*.json` sprite manifests)
- `Battle/BfkBattle.*` — deterministic battle sim; UI replays its `Events` queue
- `Run/`, `Meta/` — map gen + run state; save game, vault, breeding, milestones
- `Game/` — GameInstance (all game flow), GameMode, `BfkDemoDriver` (playtest)
- `UI/BfkWidgets.*` — style helpers, `UBfkTagButton` (payload button — UMG
  dynamic delegates can't capture), tweener; `UI/BfkFx.*` — Slate-painted
  weather/particles; `UI/Screens/` — every screen, trees built in C++

## Conventions & gotchas (hard-won)

- `BfkUi::SolidBrush` MUST stay DrawAs=RoundedBox: an Image brush with no
  resource object silently draws NOTHING on this engine build (cost a long
  debugging session — the pause overlay was "invisible" while fully laid out).
- Battle board is a 5x9 HEX FIELD (`Bfk::BoardRows/Cols` in BfkTypes.h), 5v5
  squads, odd-r offset pointy-top hexes squashed 2:1 (`CellCenter` in
  BfkBattleScreen: 200x115 tiles, origin (160,235), odd rows +100px). Hex math
  (`Bfk::HexDist`, `Bfk::HexNeighbors`) lives in BfkTypes.h. Units have Range
  and Move by archetype (`ArchetypeRange/Move`); reach weapons +1 Range. Every
  unit moves once/turn (click unit -> teal hexes); enemies out of reach
  telegraph "Advances". Energy banks between turns (cap 3); End Turn asks
  keep-or-discard hand. `CellPos` still returns a legacy CellW x CellH rect
  centered on the tile so offset math keeps working. ZOrder depth-sorts by row;
  update it when units move. Board pans (drag) / zooms (wheel) via render
  transform on BoardLayer + Particles — compose shake via ApplyBoardTransform.
- Freshly constructed UBorder widgets default to Visible — set Collapsed
  explicitly before any toggle-style SetVisibility logic.
- Creature sprites are TALL (~0.6 aspect): never draw them into fixed squares —
  use `BfkUi::SpriteFit` (aspect-preserving) for lists/grids.
- Sprite defringe: `Tools/defringe.py` (aggressive unconditional 1px shave for
  crt_/hum_/pro_/enm_/min_, relative-brightness test for the rest). Re-run it
  after re-extracting sprites, BEFORE import_assets.py.

- Asset slugs are flat under `/Game/BFK/T` and `/Game/BFK/S` with prefixes
  (crt_/hum_/pro_/enm_/min_/card_/rel_/wpn_/prj_/ui_/par_/bg_/sfx_);
  resolve via `FBfkAssets::Texture/Sound`. All character art faces LEFT —
  allies are flipped (render scale -1) to face right.
- FX overlay widgets MUST set `ESlateVisibility::HitTestInvisible` in their
  UWidget constructor (UMG SynchronizeProperties overrides Slate-level settings).
- PowerShell `| Select-Object -First N` on a piped game process KILLS it mid-run.
- The demo driver clicks by traversing the UMG tree with render-bounds hit tests
  (OS-level input fails on a locked desktop).
- Sprite pipeline (if new sheets arrive): `Tools/sprite_pipeline.py` -> label
  manifests -> `gen_content.py` -> `Tools/uepy.ps1 -Script Tools/import_assets.py`.
