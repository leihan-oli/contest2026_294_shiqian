"""Find the average face's axis of symmetry, using the region below the eyes.

The crop is centred on the pupil midpoint, so a face shot square to the
camera should be symmetric about the frame centre.  Sliding a vertical axis
across the average image and measuring how well the two halves match (one
mirrored) locates the real midline; its offset from centre is the yaw.

Only the area below the eyes is used.  The eyes themselves are asymmetric in
the left and right classes - that is the whole point of those classes - and
including them would drag the axis towards the iris, mistaking eye rotation
for head rotation.

The open class is the control: shot face-on, so its offset is the baseline
that left has to be compared against.
"""
import glob

import numpy as np
from PIL import Image

SZ = (192, 72)
LOWER = 0.50            # use rows below this fraction of the height
SEARCH = (0.30, 0.70)   # where the midline may lie


def average(pat, n=250):
    fs = sorted(glob.glob(pat))[:n]
    acc = None
    for f in fs:
        a = np.asarray(Image.open(f).convert("L").resize(SZ, Image.BILINEAR),
                       dtype=np.float32)
        acc = a if acc is None else acc + a
    return acc / len(fs)


def symmetry_axis(m):
    h, w = m.shape
    lo = m[int(h * LOWER):]
    best, bx = 1e18, w // 2
    for x in range(int(w * SEARCH[0]), int(w * SEARCH[1])):
        half = min(x, w - 1 - x)
        if half < 18:
            continue
        left = lo[:, x - half:x][:, ::-1]
        right = lo[:, x:x + half]
        d = float(np.abs(left - right).mean())
        if d < best:
            best, bx = d, x
    return (bx - (w - 1) / 2.0) / w, best


print("对称轴偏移（正=中线偏右），仅用眼睛以下区域")
print()
base = None
for name, pat in (("open 对照", "../open/open_*.bmp"),
                  ("closed 对照", "../close/closed_*.bmp"),
                  ("left", "left_*.bmp")):
    fs = glob.glob(pat)
    if not fs:
        continue
    m = average(pat)
    off, err = symmetry_axis(m)
    print("  %-12s n=%3d   偏移 %+.4f   残差 %.3f"
          % (name, min(len(fs), 250), off, err))
    if name.startswith("open"):
        base = off

if base is not None:
    print()
    print("  以 open（头正）为基准：")
    for name, pat in (("closed", "../close/closed_*.bmp"), ("left", "left_*.bmp")):
        fs = glob.glob(pat)
        if not fs:
            continue
        off, _ = symmetry_axis(average(pat))
        print("    %-8s 相对 open %+.4f  (= %.1f%% 图宽)"
              % (name, off - base, (off - base) * 100))
