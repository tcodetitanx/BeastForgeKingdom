"""Generate the isometric board tile texture (white, tinted in-engine).

Output: SourceArt/generated/ui_iso_tile.png — 512x256 diamond with a soft
translucent fill and a brighter rim, alpha-only shape (RGB stays white so
UImage::SetColorAndOpacity tints it cleanly).
"""
import numpy as np
from PIL import Image
import os

W, H = 512, 256
cx, cy = (W - 1) / 2.0, (H - 1) / 2.0

y, x = np.mgrid[0:H, 0:W]
# normalized diamond distance: 1.0 at the edge, 0 at center
d = np.abs(x - cx) / (W / 2.0) + np.abs(y - cy) / (H / 2.0)

alpha = np.zeros((H, W), dtype=np.float32)
inside = d <= 1.0
# soft fill, slightly stronger toward the rim for a beveled-slab read
alpha[inside] = 0.42 + 0.10 * d[inside]
# bright rim: a band hugging the edge
rim = inside & (d >= 0.93)
alpha[rim] = 0.95
# anti-alias the outer edge over ~1.5% of the radius
aa = (d > 1.0) & (d < 1.03)
alpha[aa] = 0.95 * np.clip((1.03 - d[aa]) / 0.03, 0, 1)

img = np.zeros((H, W, 4), dtype=np.uint8)
img[..., 0:3] = 255
img[..., 3] = np.clip(alpha * 255, 0, 255).astype(np.uint8)

out = os.path.join(os.path.dirname(__file__), "..", "SourceArt", "generated")
os.makedirs(out, exist_ok=True)
path = os.path.join(out, "ui_iso_tile.png")
Image.fromarray(img, "RGBA").save(path)
print("WROTE", os.path.abspath(path))
