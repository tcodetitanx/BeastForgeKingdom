"""Generate the isometric hex board tile (white, tinted in-engine).

Pointy-top hexagon squashed ~2:1 for the fixed-angle iso look.
Output: SourceArt/generated/ui_hex_tile.png (512x296).
"""
import numpy as np
from PIL import Image
import os

W, H = 512, 296

y, x = np.mgrid[0:H, 0:W].astype(np.float32)
u = x / (W - 1)          # 0..1
v = y / (H - 1)

# squashed pointy-top hex as intersection of half-planes:
#   |u-0.5| <= 0.5  (left/right edges)
#   slanted caps: v >= 0.25 - 0.5*(1-|2u-1|)*0.5 ... derive from vertices:
# vertices: (0.5,0) (1,0.25) (1,0.75) (0.5,1) (0,0.75) (0,0.25)
# top edges: from (0.5,0) to (1,0.25): v >= 0.25 - 0.5*(1-u)*? -> line v = 0.5*(u-0.5)... compute:
#   line through (0.5,0),(1,0.25): v = 0.5*(u-0.5) -> inside: v >= 0.5*(u-0.5)
#   line through (0.5,0),(0,0.25): v = 0.5*(0.5-u) -> inside: v >= 0.5*(0.5-u)
#   bottoms mirrored: v <= 1 - 0.5*(u-0.5), v <= 1 - 0.5*(0.5-u)
t = np.abs(u - 0.5)
top = 0.5 * t          # v of the top slanted edge at this u
bot = 1.0 - 0.5 * t
# signed distance-ish: how far inside (0 at edge, positive inside), normalized
d_top = v - top
d_bot = bot - v
d_side = 0.5 - t       # in u units
inside = (d_top >= 0) & (d_bot >= 0) & (d_side >= 0)

# normalized distance to nearest edge (rough): min of the three, scaled
dn = np.minimum(np.minimum(d_top, d_bot) * 2.0, d_side * 2.0)
dn = np.clip(dn, 0, 1)

alpha = np.zeros((H, W), dtype=np.float32)
alpha[inside] = 0.40 + 0.10 * (1.0 - dn[inside])   # slightly brighter near rim
rim = inside & (dn <= 0.055)
alpha[rim] = 0.95
# anti-alias outer edge
aa = (~inside) & (np.minimum(np.minimum(d_top, d_bot) * 2.0, d_side * 2.0) > -0.02)
alpha[aa] = 0.4 * np.clip(1.0 + np.minimum(np.minimum(d_top, d_bot) * 2.0, d_side * 2.0)[aa] / 0.02, 0, 1)

img = np.zeros((H, W, 4), dtype=np.uint8)
img[..., 0:3] = 255
img[..., 3] = np.clip(alpha * 255, 0, 255).astype(np.uint8)

out = os.path.join(os.path.dirname(__file__), "..", "SourceArt", "generated")
os.makedirs(out, exist_ok=True)
path = os.path.join(out, "ui_hex_tile.png")
Image.fromarray(img, "RGBA").save(path)
print("WROTE", os.path.abspath(path))
