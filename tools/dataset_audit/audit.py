"""Audit a capture directory for problems that are not obvious by eye.

    python3 audit.py <dir>

<dir> must hold the images and the meta.txt the board wrote.  The label is
taken from meta.txt, so the same script works for every class.

Passes:
  0. what is in the directory - labels, counts, stray files
  1. meta vs disk      - records and files that do not pair up
  2. geometry          - distance spread, eye placement drift
  3. image statistics  - exposure and focus
  4. model verdicts    - frames the float model reads differently

On pass 4: the model predates this framing (it was trained on the fixed ROI
at eye_rel 0.59, these crops are at 0.413), so its absolute verdicts are not
a pass/fail test.  The outlier pattern is the useful part - a few frames it
reads unlike the rest deserve a look whatever the cause.
"""
import glob
import os
import statistics
import sys

import numpy as np
import onnxruntime as ort
from PIL import Image

DIR = sys.argv[1] if len(sys.argv) > 1 else "."
os.chdir(DIR)

MODEL = glob.glob("/home/leihann/SoftwarePackage/*/02_中间产物/blink.onnx")[0]
CLASSES = ["closed", "open", "left", "right", "other"]
sess = ort.InferenceSession(MODEL, providers=["CPUExecutionProvider"])


def prep(img):
    im = img.convert("RGB").resize((128, 64), Image.BILINEAR)
    x = np.asarray(im, dtype=np.float32) / 255.0
    return np.transpose((x - 0.5) / 0.5, (2, 0, 1))[None].astype(np.float32)


def verdict(img):
    z = sess.run(None, {"input": prep(img)})[0][0]
    e = np.exp(z - z.max())
    p = e / e.sum()
    return int(p.argmax()), float(p.max())


raw = []
for line in open("meta.txt", encoding="utf-8"):
    if line.startswith("#") or not line.strip():
        continue
    p = line.split()
    raw.append(dict(label=p[0], seq=int(p[1]), cy=int(p[3]), w=int(p[4]),
                    h=int(p[5]), score=float(p[6]), ey=float(p[8]),
                    dx=float(p[9])))

print("== 0. 目录 %s ==" % os.path.basename(os.path.abspath(DIR)))
print("   meta labels: %s" % sorted({r["label"] for r in raw}))
stray = [f for f in os.listdir(".")
         if not f.endswith(".bmp") and f != "meta.txt"
         and not f.endswith(".py") and not f.endswith(".png")]
print("   其他文件: %s" % (stray if stray else "无"))

for LABEL in sorted({r["label"] for r in raw}):
    rows = [r for r in raw if r["label"] == LABEL]
    pre = LABEL + "_"
    files = {}
    for f in glob.glob(pre + "*.bmp"):
        try:
            files[int(f[len(pre):len(pre) + 4])] = f
        except ValueError:
            pass
    if not files:
        continue

    print()
    print("── '%s'  meta %d 条 / 磁盘 %d 个 ──" % (LABEL, len(rows), len(files)))

    got = set(files)
    onlymeta = sorted({r["seq"] for r in rows} - got)
    onlyfile = sorted(got - {r["seq"] for r in rows})
    print("   仅 meta（文件已删）: %d %s"
          % (len(onlymeta), onlymeta[:8] if onlymeta else ""))
    print("   仅文件（meta 无）  : %s" % (onlyfile[:8] if onlyfile else "无"))

    kept = [r for r in rows if r["seq"] in got]
    if not kept:
        continue

    dxs = [r["dx"] for r in kept]
    q = statistics.quantiles(dxs, n=4)
    print("   dx    %.1f .. %.1f  中位 %.1f  IQR %.1f..%.1f"
          % (min(dxs), max(dxs), statistics.median(dxs), q[0], q[2]))
    print("      最近5 %s" % ", ".join("%04d(%.0f)" % (r["seq"], r["dx"])
                                       for r in sorted(kept, key=lambda r: r["dx"])[:5]))
    print("      最远5 %s" % ", ".join("%04d(%.0f)" % (r["seq"], r["dx"])
                                       for r in sorted(kept, key=lambda r: -r["dx"])[:5]))

    rels = [(r["ey"] - r["cy"]) / r["h"] for r in kept]
    print("   eye_rel %.3f .. %.3f  中位 %.3f  (设计 0.413)"
          % (min(rels), max(rels), statistics.median(rels)))
    bad = [(r["seq"], round((r["ey"] - r["cy"]) / r["h"], 3)) for r in kept
           if abs((r["ey"] - r["cy"]) / r["h"] - 0.413) > 0.06]
    print("      偏离>0.06: %s" % (bad[:8] if bad else "无"))

    scs = [r["score"] for r in kept]
    print("   score %.3f .. %.3f  中位 %.3f" % (min(scs), max(scs), statistics.median(scs)))
    lows = [(r["seq"], r["score"]) for r in kept if r["score"] < 0.85]
    print("      <0.85 的 %d 张: %s" % (len(lows), lows[:8] if lows else "无"))

    if onlymeta:
        g = [r for r in rows if r["seq"] not in got]
        print("   [已删 %d 张的特征] score %.3f..%.3f  dx %.1f..%.1f"
              % (len(g), min(x["score"] for x in g), max(x["score"] for x in g),
                 min(x["dx"] for x in g), max(x["dx"] for x in g)))

    st = []
    for seq in sorted(files):
        a = np.asarray(Image.open(files[seq]).convert("L"), dtype=np.float32)
        st.append((seq, float(a.mean()), float(a.std()),
                   float(np.abs(np.diff(a, axis=1)).mean() +
                         np.abs(np.diff(a, axis=0)).mean())))
    for name, i in (("亮度", 1), ("对比度", 2), ("梯度", 3)):
        v = [s[i] for s in st]
        print("   %-4s %.1f .. %.1f  中位 %.1f" % (name, min(v), max(v),
                                                   statistics.median(v)))
    print("      最暗5 %s" % ", ".join("%04d(%.0f)" % (s[0], s[1])
                                       for s in sorted(st, key=lambda s: s[1])[:5]))
    print("      最糊5 %s" % ", ".join("%04d(%.2f)" % (s[0], s[3])
                                       for s in sorted(st, key=lambda s: s[3])[:5]))

    cnt = {}
    odd = []
    for seq in sorted(files):
        v, c = verdict(Image.open(files[seq]).convert("RGB"))
        cnt[CLASSES[v]] = cnt.get(CLASSES[v], 0) + 1
        if CLASSES[v] != LABEL:
            odd.append((seq, CLASSES[v], c))
    print("   旧模型判决: %s" % cnt)
    print("      非 '%s' 的 %d 张: %s"
          % (LABEL, len(odd), ", ".join("%04d->%s@%.2f" % o for o in odd[:12])))
