"""Audit the open/ captures for problems that are not obvious by eye.

Four passes:
  1. meta vs disk      - records and files that do not pair up
  2. geometry          - distance outliers, eye placement drift
  3. image statistics  - exposure and focus
  4. model verdicts    - frames the float model does not read as "open"

Note on pass 4: the model predates this framing (it was trained on the fixed
ROI at eye_rel 0.59), so its absolute verdicts are not a pass/fail test.
What is useful is the outlier pattern - a handful of frames it reads very
differently from the rest are worth a look, whatever the cause.
"""
import glob
import statistics

import numpy as np
import onnxruntime as ort
from PIL import Image

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


rows = []
for line in open("meta.txt", encoding="utf-8"):
    if line.startswith("#") or not line.strip():
        continue
    p = line.split()
    if p[0] != "open":
        continue
    rows.append(dict(seq=int(p[1]), cy=int(p[3]), w=int(p[4]), h=int(p[5]),
                     score=float(p[6]), ey=float(p[8]), dx=float(p[9])))

files = {int(f[5:9]): f for f in glob.glob("open_*.bmp")}

print("== 1. meta 与磁盘 ==")
print("   meta %d 条, 磁盘 %d 个" % (len(rows), len(files)))
onlymeta = sorted({r["seq"] for r in rows} - set(files))
onlyfile = sorted(set(files) - {r["seq"] for r in rows})
print("   仅 meta（文件已删）: %d 个 %s"
      % (len(onlymeta), onlymeta[:6] if onlymeta else ""))
print("   仅文件（meta 无）  : %s" % (onlyfile[:6] if onlyfile else "无"))
print()

print("== 2. 几何 ==")
dxs = [r["dx"] for r in rows]
q = statistics.quantiles(dxs, n=4)
print("   dx    %.1f .. %.1f   中位 %.1f   IQR %.1f..%.1f"
      % (min(dxs), max(dxs), statistics.median(dxs), q[0], q[2]))
print("      最近5: %s" % ", ".join("%04d(%.0f)" % (r["seq"], r["dx"])
                                     for r in sorted(rows, key=lambda r: r["dx"])[:5]))
print("      最远5: %s" % ", ".join("%04d(%.0f)" % (r["seq"], r["dx"])
                                     for r in sorted(rows, key=lambda r: -r["dx"])[:5]))
rels = [(r["ey"] - r["cy"]) / r["h"] for r in rows]
print("   eye_rel %.3f .. %.3f  中位 %.3f  (设计 0.413)"
      % (min(rels), max(rels), statistics.median(rels)))
bad = [(r["seq"], round((r["ey"] - r["cy"]) / r["h"], 3)) for r in rows
       if abs((r["ey"] - r["cy"]) / r["h"] - 0.413) > 0.06]
print("   偏离>0.06: %s" % (bad[:10] if bad else "无"))
scs = [r["score"] for r in rows]
print("   score %.3f .. %.3f  中位 %.3f" % (min(scs), max(scs), statistics.median(scs)))
lows = [(r["seq"], r["score"]) for r in rows if r["score"] < 0.90]
print("   <0.90 的 %d 张: %s" % (len(lows), lows[:8] if lows else "无"))
print()

print("== 3. 图像统计 ==")
st = []
for seq in sorted(files):
    a = np.asarray(Image.open(files[seq]).convert("L"), dtype=np.float32)
    st.append((seq, float(a.mean()), float(a.std()),
               float(np.abs(np.diff(a, axis=1)).mean() +
                     np.abs(np.diff(a, axis=0)).mean())))
for name, idx in (("亮度", 1), ("对比度", 2), ("梯度", 3)):
    v = [s[idx] for s in st]
    print("   %-4s %.1f .. %.1f  中位 %.1f" % (name, min(v), max(v), statistics.median(v)))
print("      最暗5: %s" % ", ".join("%04d(%.0f)" % (s[0], s[1])
                                     for s in sorted(st, key=lambda s: s[1])[:5]))
print("      最糊5: %s" % ", ".join("%04d(%.2f)" % (s[0], s[3])
                                     for s in sorted(st, key=lambda s: s[3])[:5]))
print()

print("== 4. 旧模型判决 ==")
cnt = {}
odd = []
for seq in sorted(files):
    v, c = verdict(Image.open(files[seq]).convert("RGB"))
    cnt[CLASSES[v]] = cnt.get(CLASSES[v], 0) + 1
    if CLASSES[v] != "open":
        odd.append((seq, CLASSES[v], c))
print("   %s" % cnt)
print("   非 open 的 %d 张: %s"
      % (len(odd), ", ".join("%04d->%s@%.2f" % o for o in odd[:14])))
