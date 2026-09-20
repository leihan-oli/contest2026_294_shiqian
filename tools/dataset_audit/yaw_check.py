"""Head-yaw check for capture classes, using the region below the eyes.

    python3 yaw_check.py left right ...

Two measurements per class:

  average face   stack every frame, take the horizontal-gradient image (edges
                 rather than brightness, so the lighting falloff does not
                 dominate), then slide a fixed-width vertical axis and find
                 where the two halves match best when one is mirrored.  The
                 crop is centred on the pupil midpoint, so a square-on face
                 puts that axis at the frame centre; the offset is the yaw.

  frame spread   the same per frame on a sample.  A class where every frame
                 carries the same yaw teaches the model that angle and fails
                 when someone holds still and only moves their eyes, so the
                 spread matters more than the mean.

Everything below 65% of the height only.  The eyes are the asymmetric part in
the left and right classes; including them drags the axis towards the iris and
turns eye rotation into apparent head rotation - which is exactly the mistake
this script exists to avoid.
"""
import glob
import os
import random
import statistics
import sys

import numpy as np
from PIL import Image

SZ = (192, 72)
LOWER = 0.65
N_SAMPLE = 60


def avg_grad(fs, n=250):
    acc = None
    for f in fs[:n]:
        a = np.asarray(Image.open(f).convert("L").resize(SZ, Image.BILINEAR),
                       dtype=np.float32)
        g = np.abs(np.diff(a, axis=1))
        acc = g if acc is None else acc + g
    return acc / min(len(fs), n)


def axis_of(m, rows_from=0.0):
    h, w = m.shape
    lo = m[int(h * rows_from):]
    half = w // 3
    cen = (w - 1) / 2.0
    best, bx = 1e18, int(cen)
    for x in range(half, w - half):
        left = lo[:, x - half:x][:, ::-1]
        right = lo[:, x:x + half]
        d = float(np.abs(left - right).mean())
        if d < best:
            best, bx = d, x
    return (bx - cen) / w, best


def per_frame(f):
    a = np.asarray(Image.open(f).convert("L").resize(SZ, Image.BILINEAR),
                   dtype=np.float32)
    g = np.abs(np.diff(a, axis=1))
    return axis_of(g, rows_from=LOWER)[0]


random.seed(42)

def collect(d):
    fs = sorted(glob.glob(os.path.join(d, "*.bmp")))
    return [f for f in fs if os.path.basename(f).split("_")[0] in
            ("open", "closed", "left", "right", "other")]


print("== 平均图对称轴（正 = 中线偏右；仅眼睛以下，避开虹膜）==")
res = {}
for d in sys.argv[1:]:
    fs = collect(d)
    if not fs:
        print("  %-10s 无数据" % os.path.basename(d))
        continue
    off, err = axis_of(avg_grad(fs))
    res[os.path.basename(d)] = off
    print("  %-10s n=%3d   偏移 %+.4f   残差 %.3f" % (os.path.basename(d), len(fs), off, err))

base = None
for k in res:
    if k.startswith("open") or k.startswith("closed"):
        base = res[k]
        break
if base is not None:
    print()
    print("  以头正类为基准（取第一个 open/closed）:")
    for k, v in res.items():
        if k == "open" or k == "closed":
            continue
        print("    %-10s 相对 %+.4f  = %.1f%% 图宽" % (k, v - base, (v - base) * 100))

print()
print("== 逐帧头姿变化（采样 %d 帧）==" % N_SAMPLE)
for d in sys.argv[1:]:
    fs = collect(d)
    if not fs:
        continue
    if len(fs) > N_SAMPLE:
        fs = random.sample(fs, N_SAMPLE)
    v = [per_frame(f) for f in fs]
    print("  %-10s 中位 %+.4f  标准差 %.4f   IQR %+.3f..%+.3f"
          % (os.path.basename(d), statistics.median(v), statistics.stdev(v),
             statistics.quantiles(v, n=4)[0], statistics.quantiles(v, n=4)[2]))
