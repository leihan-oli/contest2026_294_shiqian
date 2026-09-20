"""Average the horizontal-gradient image and find its axis of symmetry.

Two fixes over the first attempt:

  * edges, not brightness - the lighting falls off to one side of the frame
    (a window is behind you, not a studio light), and that gradient is not
    symmetric, so fitting brightness alone just finds the lamp
  * a fixed-width comparison window - the first version used
    min(x, w-1-x), which shrinks near the edges and made those positions
    look artificially well matched, pinning the answer to the search bound

Only rows below the eyes go in, because the eyes are the asymmetric part in
the left and right classes.
"""
import glob

import numpy as np
from PIL import Image

SZ = (192, 72)
LOWER = 0.65


def average_grad(pat, n=250):
    fs = sorted(glob.glob(pat))[:n]
    acc = None
    for f in fs:
        a = np.asarray(Image.open(f).convert("L").resize(SZ, Image.BILINEAR),
                       dtype=np.float32)
        g = np.abs(np.diff(a, axis=1))          # one column narrower
        acc = g if acc is None else acc + g
    return acc / len(fs)


def symmetry_axis(m):
    h, w = m.shape
    lo = m[int(h * LOWER):]
    half = w // 3
    cen = (w - 1) / 2.0
    best, bx = 1e18, int(cen)
    for x in range(half, w - half):
        left = lo[:, x - half:x][:, ::-1]
        right = lo[:, x:x + half]
        d = float(np.abs(left - right).mean())
        if d < best:
            best, bx = d, x
    return (bx - cen) / w, best, bx, w


print("对称轴偏移（正 = 中线偏右），梯度图 + 固定窗口")
print()
res = {}
for name, pat in (("open 对照", "../open/open_*.bmp"),
                  ("closed 对照", "../close/closed_*.bmp"),
                  ("left", "left_*.bmp")):
    fs = glob.glob(pat)
    if not fs:
        continue
    off, err, bx, w = symmetry_axis(average_grad(pat))
    res[name] = off
    print("  %-12s n=%3d   偏移 %+.4f   残差 %.3f   (轴在列 %d/%d)"
          % (name, min(len(fs), 250), off, err, bx, w))

print()
base = None
for k in res:
    if k.startswith("open"):
        base = res[k]
if base is not None:
    print("  以 open（头正）为基准：")
    for k in ("closed 对照", "left"):
        if k in res:
            d = res[k] - base
            print("    %-12s %+.4f   = %.1f%% 图宽   ≈ %.1f 度（按脸宽 ~1.6 倍图宽估）"
                  % (k, d, d * 100, abs(d) * 100 / 1.6))
