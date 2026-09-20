"""Estimate head yaw from where the nose bridge sits in the frame.

The crop is centred on the pupil midpoint, so with the head square to the
camera the bridge sits near the middle of the frame; a yawed head pushes it
to one side.  Crude, but it separates "head a bit off" from "head square on"
well enough to judge usability.

Measures the darkest column of the lower half - below the eyes, where the
bridge and its shadow are - and reports the offset from centre as a fraction
of width.  The open class is measured alongside as the reference: those were
shot facing the camera.

Note the sign is arbitrary (depends on where the shadow falls), so read the
spread, not the sign.
"""
import glob
import statistics

import numpy as np
from PIL import Image


def bridge_offset(path):
    a = np.asarray(Image.open(path).convert("L"), dtype=np.float32)
    h, w = a.shape
    lower = a[int(h * 0.55):]
    col = lower.mean(axis=0)
    k = max(3, w // 20)
    sm = np.convolve(col, np.ones(k) / k, mode="same")
    x = int(sm.argmin())
    return (x - (w - 1) / 2.0) / w


for name, pat in (("left", "left_*.bmp"),
                  ("open 对照", "../open/open_*.bmp"),
                  ("closed 对照", "../close/closed_*.bmp")):
    fs = sorted(glob.glob(pat))
    if not fs:
        continue
    offs = [(f, bridge_offset(f)) for f in fs]
    v = [o for _, o in offs]
    print("%-12s n=%3d   中位 %+.3f   均值 %+.3f   范围 %+.3f .. %+.3f   |中位| %.3f"
          % (name, len(v), statistics.median(v), statistics.fmean(v),
             min(v), max(v), abs(statistics.median(v))))
    ext = sorted(offs, key=lambda t: -abs(t[1]))[:5]
    print("             最偏 5: %s"
          % "  ".join("%s(%+.2f)" % (f.split("/")[-1][5:9], o) for f, o in ext))
