"""Per-frame yaw estimate: is the head angle stuck, or does it wander?

This matters more than the average.  A class where every frame carries the
same head yaw teaches the model that angle - it will pass on these captures
and fail the moment someone holds their head still and only moves their
eyes, which is the actual use case.  A class where the yaw wanders forces
the model onto the iris instead.

Samples frames and reports the spread.  Single frames are noisier than the
averaged images (glasses, shadows), so read the spread, not each value.
"""
import glob
import random
import statistics

import numpy as np
from PIL import Image

SZ = (192, 72)
LOWER = 0.65
N = 60


def yaw_of(path):
    a = np.asarray(Image.open(path).convert("L").resize(SZ, Image.BILINEAR),
                   dtype=np.float32)
    g = np.abs(np.diff(a, axis=1))
    lo = g[int(g.shape[0] * LOWER):]
    h, w = lo.shape
    half = w // 4
    cen = (w - 1) / 2.0
    best, bx = 1e18, int(cen)
    for x in range(half, w - half):
        left = lo[:, x - half:x][:, ::-1]
        right = lo[:, x:x + half]
        d = float(np.abs(left - right).mean())
        if d < best:
            best, bx = d, x
    return (bx - cen) / w


random.seed(42)
print("逐帧头偏估计（采样 %d 帧/类）" % N)
print()
for name, pat in (("open 对照", "../open/open_*.bmp"),
                  ("closed 对照", "../close/closed_*.bmp"),
                  ("left", "left_*.bmp")):
    fs = sorted(glob.glob(pat))
    if not fs:
        continue
    if len(fs) > N:
        fs = random.sample(fs, N)
    v = [yaw_of(f) for f in fs]
    print("  %-12s 中位 %+.4f  均值 %+.4f  标准差 %.4f  IQR %+.3f..%+.3f"
          % (name, statistics.median(v), statistics.fmean(v), statistics.stdev(v),
             statistics.quantiles(v, n=4)[0], statistics.quantiles(v, n=4)[2]))
