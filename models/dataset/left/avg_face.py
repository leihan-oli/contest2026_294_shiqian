"""Average face: stack many captures to see the framing they share.

Averaging removes lighting, glasses glare and frame-to-frame variation,
leaving what the captures have in common.  If the head was yawed while
shooting the left class, the average nose bridge will sit off the average
eye centre - and the open class, shot face-on, gives the baseline to compare
against.

The columns to the far left and right are ignored when locating the bridge:
those are cheek and frame edge, which are darker than the bridge and would
win the argmin otherwise.
"""
import glob

import numpy as np
from PIL import Image

SZ = (192, 72)          # w, h - common canvas
EDGE = 0.12             # fraction of width to ignore at each side


def average(pat, n=250):
    fs = sorted(glob.glob(pat))[:n]
    acc = None
    for f in fs:
        a = np.asarray(Image.open(f).convert("L").resize(SZ, Image.BILINEAR),
                       dtype=np.float32)
        acc = a if acc is None else acc + a
    return acc / len(fs)


def bridge(m):
    h, w = m.shape
    lo = m[int(h * 0.55):]
    col = lo.mean(axis=0)
    i0, i1 = int(w * EDGE), int(w * (1 - EDGE))
    x = i0 + int(col[i0:i1].argmin())
    k = max(3, w // 20)
    sm = np.convolve(col, np.ones(k) / k, mode="same")
    xs = i0 + int(sm[i0:i1].argmin())
    return (x - (w - 1) / 2.0) / w, (xs - (w - 1) / 2.0) / w, m


tiles = []
for name, pat in (("left", "left_*.bmp"),
                  ("open", "../open/open_*.bmp"),
                  ("closed", "../close/closed_*.bmp")):
    fs = glob.glob(pat)
    if not fs:
        continue
    m = average(pat)
    raw, smooth, _ = bridge(m)
    print("%-8s n=%3d   鼻梁列偏移  原始 %+.3f   平滑 %+.3f"
          % (name, min(len(fs), 250), raw, smooth))
    tiles.append((name, m))

Z = 3
W = SZ[0]
H = SZ[1] * len(tiles) + 4 * (len(tiles) - 1)
c = Image.new("RGB", (W * Z, H * Z), (40, 40, 40))
y = 0
for name, m in tiles:
    im = Image.fromarray(m.astype(np.uint8)).convert("RGB")
    c.paste(im.resize((W * Z, SZ[1] * Z), Image.NEAREST), (0, y))
    y += SZ[1] * Z + 4 * Z
c.save("avg_faces.png")
print("  -> avg_faces.png %dx%d  (上到下: %s)"
      % (c.size[0], c.size[1], ", ".join(t[0] for t in tiles)))
