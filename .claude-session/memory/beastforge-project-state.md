---
name: beastforge-project-state
description: "BeastForge Kingdom UE 5.8 game — architecture, tooling, and gotchas from the full build session"
metadata: 
  node_type: memory
  type: project
  originSessionId: 35461404-6a06-4176-8db1-96ac51b332cf
---

BeastForge Kingdom (D:\UEprojects\BeastForgeKingdom) is a complete 3v3 deck-builder
creature-collector roguelite built 2026-07-01, pushed to github.com/tcodetitanx/BeastForgeKingdom.

- Engine: UE 5.8 **source build at D:/UE_5.8** (registry GUID association). Experimental
  ModelContextProtocol plugin enabled (HTTP JSON-RPC localhost:8000/mcp, auto-start via
  Config/DefaultEditorPerProjectUserSettings.ini; client: Tools/mcp.ps1).
- All logic is C++ (no Blueprints). Content (285 species/400+ cards/128 weapons/80 relics) is
  GENERATED from sprite-label JSONs: Tools/labels/*.json -> Tools/gen_content.py ->
  BfkContentGenerated.cpp. Never hand-edit that file; rerun the generator.
- Sprite pipeline: Tools/sprite_pipeline.py (flood-fill white removal + edge unmixing +
  fragment adoption) -> SourceArt/sprites; import via Tools/uepy.ps1 + import_assets.py to
  /Game/BFK/T (textures, flat, prefixed slugs like crt_/enm_/ui_/par_) and /Game/BFK/S (sfx).
- Scripted playtesting: launch game with -BfkDemo=<script> (wait/click/shot/exec/quit,
  1920x1080 design coords). Clicks are dispatched by TRAVERSING THE UMG TREE
  (GetRenderBoundingRect hit test) because Slate/OS input synthesis fails when the Windows
  session is locked. Console helpers: Bfk.NewRun <seed>, Bfk.Enter <nodetype>.
- Tests: `-ExecCmds="Automation RunTests BeastForge; Quit" -unattended -nullrhi` (4 suites).
- Gotchas hit: PowerShell `| Select-Object -First N` KILLS a piped game process mid-run;
  UMG SynchronizeProperties overrides Slate-level SetVisibility (set on UWidget instead);
  FX overlay widgets must be HitTestInvisible in their UWidget constructor; menu screen
  composites live widgets over the painted bg_main_menu mockup with opaque cover panels.
- Related: [[beastforge-user-prefs]]
