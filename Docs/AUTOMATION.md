# Engine automation toolbelt

Everything the project uses to drive UE 5.8 without a human at the keyboard.

## Build & scripting

| Tool | What it does |
|---|---|
| `Tools/build.ps1` | Compile the editor target (UBT). `-Game` for the game target. |
| `Tools/uepy.ps1 -Script <py>` | Run a Python script headless inside the editor (commandlet). |
| `Tools/export_textures.py` | Export /Game/images texture uassets back to PNG. |
| `Tools/sprite_pipeline.py` | White-bg removal + sprite slicing + contact sheets. |
| `Tools/gen_content.py` | Labels JSON -> BfkContentGenerated.cpp + import manifest. |
| `Tools/import_assets.py` | Import sprites + curated SFX into /Game/BFK. |
| `Tools/make_boot_map.py` | Create the empty Boot level. |
| `Tools/prune_audio.py` | Delete unused wavs + Unity .meta from Content/soundFX. |

## MCP

The engine's experimental Model Context Protocol server is enabled
(`Config/DefaultEditorPerProjectUserSettings.ini`, auto-start, port 8000).
`.mcp.json` registers it for MCP clients; `Tools/mcp.ps1` is a standalone
JSON-RPC client (`init` / `list` / `call`) for editor sessions.

## Scripted playtesting (-BfkDemo)

Launch the game with `-BfkDemo=<script.txt>` to drive the real UI and capture
engine screenshots (works on a locked desktop — clicks are dispatched straight
into the UMG tree, screenshots via FScreenshotRequest):

```
wait 2.5          # seconds
click 960 930     # 1920x1080 design-space coords
rclick 400 500
shot menu         # -> Saved/Screenshots/WindowsEditor/menu.png
exec Bfk.NewRun 12345   # any console command
quit
```

Console helpers for deterministic tours:
- `Bfk.NewRun [seed]` — start a run with the first 3 vault beasts.
- `Bfk.Enter [type]` — enter a reachable map node (prefer a type substring).

Demo scripts: `demo_intro.txt`, `demo_battle.txt`, `demo_screens.txt`.

## Tests

`UnrealEditor-Cmd <proj> -ExecCmds="Automation RunTests BeastForge; Quit" -unattended -nullrhi`

- `BeastForge.Content.Sanity` — registries complete, kits resolve, bosses exist.
- `BeastForge.Battle.RandomSim` — 60 randomized battles play to termination.
- `BeastForge.Run.MapGen` — 100 maps: connectivity, single reachable boss.
- `BeastForge.Meta.Breeding` — compatibility, egg creation, hatching.
