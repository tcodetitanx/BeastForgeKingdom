"""Delete unused audio from Content/soundFX (keep only files imported as game SFX)
plus strip all Unity .meta files. Run AFTER import_assets.py has been executed."""
import os, sys, re

ROOT = r"D:/UEprojects/BeastForgeKingdom"
SFX_BASE = os.path.join(ROOT, "Content", "soundFX", "Fantasy_Game_16bit")

# parse the used-file list straight from import_assets.py (single source of truth)
used = set()
src = open(os.path.join(ROOT, "Tools", "import_assets.py"), encoding="utf-8").read()
for m in re.finditer(r'"sfx_[a-z_]+":\s*"([^"]+)"', src):
    used.add(os.path.normpath(os.path.join(SFX_BASE, m.group(1))))

removed = kept = 0
for dirpath, _dirs, files in os.walk(SFX_BASE, topdown=False):
    for f in files:
        p = os.path.normpath(os.path.join(dirpath, f))
        if f.lower().endswith(".meta"):
            os.remove(p); removed += 1
        elif f.lower().endswith(".wav"):
            if p in used:
                kept += 1
            else:
                os.remove(p); removed += 1
        else:
            kept += 1
    # remove now-empty dirs
    try:
        os.rmdir(dirpath)
    except OSError:
        pass

print(f"kept {kept} files, removed {removed}")
