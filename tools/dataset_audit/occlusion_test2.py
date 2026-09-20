"""Occlusion test, second pass - bands cut by what is actually in them.

The first pass used equal thirds, which is wrong for this framing: the crop
height is 0.967*dx while the eye is about 0.248*dx tall, so with the eye
centre at 0.307 the eyes occupy y = 0.183 .. 0.431.  The "top" third
therefore contained the upper half of the eyes, which is why it scored
highest - not because the forehead matters.

Bands here follow the content:
    forehead  0.00 .. 0.18     above the eyes
    eyes      0.18 .. 0.43     the eyes themselves
    cheek     0.43 .. 0.66     below the eyes, above the nose
    nose      0.66 .. 1.00     the nostrils

Two fills are used.  The mean colour is neutral but forms a flat band; pure
black is a strong signal.  If a band's score depends heavily on which fill
was used, the number says more about the fill than about the band.
"""
import glob
import sys

import numpy as np
import onnxruntime as ort
from PIL import Image

_cands = glob.glob("/home/leihann/SoftwarePackage/*/02_中间产物/blink.onnx")
assert _cands, "blink.onnx not found"
MODEL = _cands[0]
CLASSES = ["closed", "open", "left", "right", "other"]

BANDS = [
    ("forehead", 0.00, 0.18),
    ("eyes", 0.18, 0.43),
    ("cheek", 0.43, 0.66),
    ("nose", 0.66, 1.00),
]

sess = ort.InferenceSession(MODEL, providers=["CPUExecutionProvider"])


def prep(img):
    im = img.convert("RGB").resize((128, 64), Image.BILINEAR)
    x = np.asarray(im, dtype=np.float32) / 255.0
    x = (x - 0.5) / 0.5
    return np.transpose(x, (2, 0, 1))[None].astype(np.float32)


def run(img):
    logits = sess.run(None, {"input": prep(img)})[0][0]
    e = np.exp(logits - logits.max())
    return e / e.sum()


def mask(img, lo, hi, fill):
    a = np.asarray(img.convert("RGB")).astype(np.float32)
    h = a.shape[0]
    y0 = int(round(h * lo))
    y1 = int(round(h * hi))
    if y1 <= y0:
        y1 = y0 + 1
    if fill == "mean":
        a[y0:y1] = a.reshape(-1, 3).mean(axis=0)
    else:
        a[y0:y1] = 0.0
    return Image.fromarray(a.astype(np.uint8))


files = sorted(glob.glob("test_*.bmp"))
print("模型: %s" % MODEL.split("/")[-1])
print("图: %d 张 test_" % len(files))
print()
print("分带依据: crop_h = 0.967*dx, 眼高约 0.248*dx, 眼心 0.307")
print("        -> 眼睛占 y = 0.183 .. 0.431")
print()

acc = {}
for fill in ("mean", "black"):
    for name, lo, hi in BANDS:
        acc[(fill, name)] = []

for f in files:
    img = Image.open(f).convert("RGB")
    base = run(img)
    print("── %s (%dx%d)  base=%s" % (
        f, img.width, img.height,
        " ".join("%s %.3f" % (c[:4], v) for c, v in zip(CLASSES, base))))
    for fill in ("mean", "black"):
        row = []
        for name, lo, hi in BANDS:
            p = run(mask(img, lo, hi, fill))
            d = float(np.abs(p - base).sum())
            acc[(fill, name)].append(d)
            row.append("%s=%.3f" % (name, d))
        print("   fill=%-6s %s" % (fill, "  ".join(row)))
    print()

print("══ %d 张平均 L1 距离 ══" % len(files))
print("%-10s %10s %10s %10s" % ("遮挡带", "mean填充", "black填充", "两者差"))
for name, lo, hi in BANDS:
    a = float(np.mean(acc[("mean", name)]))
    b = float(np.mean(acc[("black", name)]))
    print("%-10s %10.3f %10.3f %10.3f" % (name, a, b, abs(a - b)))

print()
print("══ 判据 ══")
ey = float(np.mean(acc[("mean", "eyes")]))
for name, lo, hi in BANDS:
    if name == "eyes":
        continue
    m = float(np.mean(acc[("mean", name)]))
    ratio = m / ey if ey > 0 else 0.0
    print("%-10s 相对眼睛区 %.0f%%  %s"
          % (name, ratio * 100,
             "影响很小" if ratio < 0.15 else
             "影响不大" if ratio < 0.35 else
             "影响明显" if ratio < 0.7 else "影响很大"))
