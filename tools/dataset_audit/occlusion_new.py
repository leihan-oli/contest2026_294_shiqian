"""Verify the new framing with the occlusion test.

Frames 11-15 were captured with the bottom edge raised a quarter, so the
nostrils should now be outside the crop.  Bands are cut from each frame's
own detected eye position rather than assuming a fixed layout, so they
follow the subject.

What to look for: "below" should now score far lower than it did on the old
framing (where it was 20% of the eyes), because what is left down there is
cheek and glasses frame, not nose.
"""
import glob

import numpy as np
import onnxruntime as ort
from PIL import Image

_cands = glob.glob("/home/leihann/SoftwarePackage/*/02_中间产物/blink.onnx")
assert _cands, "blink.onnx not found"
MODEL = _cands[0]
CLASSES = ["closed", "open", "left", "right", "other"]

FILES = ["test_%04d.bmp" % i for i in range(11, 16)]
EYE_HALF = 0.175          # half the eye height, as a fraction of crop height

rel = {}
for line in open("meta.txt", encoding="utf-8"):
    if not line.startswith("test"):
        continue
    p = line.split()
    seq = int(p[1])
    if seq >= 11:
        rel[seq] = (float(p[8]) - float(p[3])) / float(p[5])

sess = ort.InferenceSession(MODEL, providers=["CPUExecutionProvider"])


def prep(img):
    im = img.convert("RGB").resize((128, 64), Image.BILINEAR)
    x = np.asarray(im, dtype=np.float32) / 255.0
    return np.transpose((x - 0.5) / 0.5, (2, 0, 1))[None].astype(np.float32)


def run(img):
    z = sess.run(None, {"input": prep(img)})[0][0]
    e = np.exp(z - z.max())
    return e / e.sum()


def mask(img, lo, hi):
    a = np.asarray(img.convert("RGB")).astype(np.float32)
    h = a.shape[0]
    y0 = int(round(h * max(0.0, lo)))
    y1 = int(round(h * min(1.0, hi)))
    if y1 <= y0:
        y1 = y0 + 1
    a[y0:y1] = a.reshape(-1, 3).mean(axis=0)
    return Image.fromarray(a.astype(np.uint8))


print("模型: %s" % MODEL.split("/")[-1])
print("新构图（下界上推 1/4）：test_0011..0015")
print()

acc = {"forehead": [], "eyes": [], "below": []}
for f in FILES:
    seq = int(f[5:9])
    e = rel.get(seq, 0.413)
    img = Image.open(f).convert("RGB")
    base = run(img)

    parts = []
    for name, lo, hi in [("forehead", 0.0, e - EYE_HALF),
                         ("eyes", e - EYE_HALF, e + EYE_HALF),
                         ("below", e + EYE_HALF, 1.0)]:
        d = float(np.abs(run(mask(img, lo, hi)) - base).sum())
        acc[name].append(d)
        parts.append("%s=%.3f" % (name, d))

    print("  %s  eye_rel=%.3f  带 %.0f..%.0f%%"
          % (f, e, (e - EYE_HALF) * 100, (e + EYE_HALF) * 100))
    print("      %s" % "  ".join(parts))
    print("      base  %s" % " ".join("%s %.3f" % (c[:4], v)
                                      for c, v in zip(CLASSES, base)))

print()
print("══ 平均 L1 ══")
ey = float(np.mean(acc["eyes"]))
for name in ("eyes", "forehead", "below"):
    m = float(np.mean(acc[name]))
    note = "(基准)" if name == "eyes" else "相对眼睛区 %.0f%%" % (m / ey * 100)
    print("  %-10s %.3f   %s" % (name, m, note))

print()
print("══ 与旧构图对照（occlusion_test2.py 的同一模型）══")
print("  旧: eye_rel 0.31，下方含鼻孔，'nose' 区 = 0.375 = 眼睛区的 20%")
print("  新: 见上，'below' 区应显著更低")
