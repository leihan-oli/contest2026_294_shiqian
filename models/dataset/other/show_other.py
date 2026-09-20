"""Sample the other class across its extremes: dark, bright, flat, busy.

Unlike the other classes these are not eyes, so "quality" means coverage -
do the frames span what the application will actually see when no eye is in
view?  Look at what is in them rather than how sharp they are.
"""
import glob

import numpy as np
from PIL import Image

st = []
for f in sorted(glob.glob("other_*.bmp")):
    a = np.asarray(Image.open(f).convert("L"), dtype=np.float32)
    g = float(np.abs(np.diff(a, axis=1)).mean() + np.abs(np.diff(a, axis=0)).mean())
    st.append((f, float(a.mean()), float(a.std()), g))

picks = []
picks += [(f, "最暗 %.0f" % m) for f, m, _, _ in sorted(st, key=lambda t: t[1])[:2]]
picks += [(f, "最亮 %.0f" % m) for f, m, _, _ in sorted(st, key=lambda t: -t[1])[:2]]
picks += [(f, "最平(对比度%.0f)" % s) for f, _, s, _ in sorted(st, key=lambda t: t[2])[:2]]
picks += [(f, "最糊 %.1f" % g) for f, _, _, g in sorted(st, key=lambda t: t[3])[:2]]
picks += [(f, "最清 %.1f" % g) for f, _, _, g in sorted(st, key=lambda t: -t[3])[:2]]

# de-dup, keep order
seen = set()
uniq = []
for f, t in picks:
    if f not in seen:
        seen.add(f)
        uniq.append((f, t))
picks = uniq

ims = [Image.open(f).convert("RGB") for f, _ in picks]
Z = 3
W = max(i.width for i in ims)
H = sum(i.height for i in ims) + 5 * (len(ims) - 1)

c = Image.new("RGB", (W * Z, H * Z), (40, 40, 40))
y = 0
for (f, t), im in zip(picks, ims):
    c.paste(im.resize((im.width * Z, im.height * Z), Image.NEAREST), (0, y))
    y += im.height * Z + 5 * Z
    print("  %-18s %-16s %dx%d" % (f, t, im.width, im.height))

c.save("check_other.png")
print("  -> check_other.png %dx%d" % c.size)
print()
print("  亮度中位 %.0f  对比度中位 %.0f  梯度中位 %.1f  (n=%d)"
      % (sorted(t[1] for t in st)[len(st) // 2],
         sorted(t[2] for t in st)[len(st) // 2],
         sorted(t[3] for t in st)[len(st) // 2], len(st)))
