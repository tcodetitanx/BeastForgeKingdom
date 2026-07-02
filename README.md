# BeastForge Kingdom

A 3v3 deck-building creature-collector roguelite for Unreal Engine 5.8, set in the
drowned kingdom of Vhal'Mora. Slay the Spire's run structure, Into the Breach's
positional chess, and a persistent creature collection & breeding meta.

![genre](https://img.shields.io/badge/genre-dark%20fantasy%20roguelite-2b3a42)

## The pitch

The BeastForge — the furnace that bound beast souls to wardens — was drowned by
its own grieving king. You are the last Forgewarden. Snare feral beasts, breed
hearthborn ones, arm your squad of three, and take the four Embers back from the
Wardens-that-Were.

- **Your squad IS your deck** — each of your 3 fighters contributes its
  signature cards; weapons add more; relics warp them. Change the squad, change
  the game. Enemy decks are built the same way.
- **Position matters** — 3x4 tactical board, lane attacks, shoves, collisions,
  powder kegs, hazard cells, telegraphed intents you can dodge or body-block.
- **Permanent progression** — captures and bred eggs stay in your Vault across
  runs and deaths. Weapons, relics and milestones unlock forever.
- **Weather is mechanical** — rain dampens Ember and conducts Shock through
  puddles; snow chills your back line; ashfall smothers healing.
- **Modes** — 4-act campaign with unique bosses, single Local Battles, and
  hotseat PvP.

## Building

Requires UE 5.8 (source or installed) associated with the project.

```powershell
Tools\build.ps1              # compile the editor target
Tools\uepy.ps1 -Script Tools/import_assets.py   # (re)import art + sfx
```

Run tests: `UnrealEditor-Cmd BeastForgeKingdom.uproject -ExecCmds="Automation RunTests BeastForge" -unattended -nullrhi`

## Project layout

- `Source/BeastForgeKingdom/Core` — types, content registries (285 species,
  400+ cards, 128 weapons, 80 relics — generated from labeled sprite manifests)
- `Source/BeastForgeKingdom/Battle` — deterministic battle model (pure sim, UI
  replays its event queue)
- `Source/BeastForgeKingdom/Run` — act map generation, encounters, events
- `Source/BeastForgeKingdom/Meta` — profile, vault, breeding, milestones
- `Source/BeastForgeKingdom/UI` — the whole interface, built in C++ (no
  Blueprint), incl. procedural weather/particle rendering in Slate OnPaint
- `Tools/` — sprite extraction pipeline (white-bg removal, slicing,
  classification manifests) and engine automation scripts
- `Docs/` — design doc and lore bible

## Credits

- Art generated for this project (sprite sheets processed by `Tools/sprite_pipeline.py`)
- SFX: Fantasy Game 16bit pack (licensed)
- Fonts: Cardo & Pirata One (SIL OFL, via Google Fonts)
