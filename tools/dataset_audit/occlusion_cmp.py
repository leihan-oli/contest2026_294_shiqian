"""Occlusion, both framings, same method - so the numbers can be compared.

The earlier passes cannot be compared with each other: one used equal
thirds, another used bands derived from the eye position.  Here both the
old frames (1-5, eye_rel 0.31) and the new ones (11-15, eye_rel 0.414) are
cut with bands derived from their own detected eye position.

Prints what the model decides after each mask, not just the distance: a
large delta that keeps the same top class means something different from a
small delta that flips it.
"""
import glob

import numpy as np
import onnxruntime as ort
from PIL import Image

_cands = glob.glob("/home/leihann/SoftwarePackage/*/02_中间产物/blink.onnx")
assert _cands, "blink.onnx not found"
MODEL = _cands[0]
CLASSES = ["closed", "open", "left", "right", "other"]

EYE_HALF = 0.175          # half the eye height, as a fraction of crop height

GROUPS = [("old 0001-0005 (eye_rel 0.31)", [1, 2, 3, 4, 5]),
          ("new 0011-0015 (eye_rel 0.414)", [11, 12, 13, 14, 15])]

rel = {}
for line in open("meta.txt", encoding="utf-8"):
    if not line.startswith("test"):
        continue
    p = line.split()
    rel[int(p[1])] = (float(p[8]) - float(p[3])) / float(p[5])

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


def top(p):
    i = int(p.argmax())
    return "%s %.2f" % (CLASSES[i][:5], p[i])


print("模型: %s" % MODEL.split("/")[-1])
print()

for title, seqs in GROUPS:
    print("══ %s ══" % title)
    acc = {"eyes": [], "forehead": [], "below": []}
    for s in seqs:
        f = "test_%04d.bmp" % s
        e = rel[s]
        img = Image.open(f).convert("RGB")
        base = run(img)

        parts = []
        for name, lo, hi in [("forehead", 0.0, e - EYE_HALF),
                             ("eyes", e - EYE_HALF, e + EYE_HALF),
                             ("below", e + EYE_HALF, 1.0)]:
            p = run(mask(img, lo, hi))
            d = float(np.abs(p - base).sum())
            acc[name].append(d)
            parts.append("%-8s Δ=%.3f -> %s" % (name, d, top(p)))

        print("  %s  h=%d  eye_rel=%.3f  base=%s"
              % (f, img.height, e, top(base)))
        for t in parts:
            print("      %s" % t)

    ey = float(np.mean(acc["eyes"]))
    print("  ── 平均: eyes=%.3f  forehead=%.3f (%.0f%%)  below=%.3f (%.0f%%)"
          % (ey,
             float(np.mean(acc["forehead"])),
             float(np.mean(acc["forehead"])) / ey * 100,
             float(np.mean(acc["below"])),
             float(np.mean(acc["below"])) / ey * 100))
    print()
