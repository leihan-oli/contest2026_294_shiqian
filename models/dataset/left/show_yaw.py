"""Left captures: did the head turn, and did the eyes turn with it?

A left/right classifier should key on the iris position inside the eye - the
pale sclera wedge on one side.  If the head turned instead, the eyes may
barely have rotated and the wedge is not what the picture shows; worse, a
model trained on such frames learns head yaw, which fails the moment the
user keeps their head still and moves only the eyes.

The open-class frame at the end is the reference: head square to the camera.
"""
import os

from PIL import Image

CWD = os.path.dirname(os.path.abspath(__file__))
REF = os.path.normpath(os.path.join(CWD, "..", "open", "open_0001.bmp"))

PICK = [
    ("left_0001.bmp", "判 open@1.00"),
    ("left_0002.bmp", "判 open@0.99"),
    ("left_0010.bmp", "判 open@1.00"),
    ("left_0020.bmp", "判 open"),
    ("left_0044.bmp", "判 closed@0.77"),
    ("left_0094.bmp", "判 closed@0.87"),
    (REF,             "对照 open 类(头正)"),
]

pick = [(f, t) for f, t in PICK if os.path.exists(f)]
ims = [Image.open(f).convert("RGB") for f, _ in pick]

Z = 3
W = max(i.width for i in ims)
H = sum(i.height for i in ims) + 6 * (len(ims) - 1)

c = Image.new("RGB", (W * Z, H * Z), (40, 40, 40))
y = 0
for (f, t), im in zip(pick, ims):
    c.paste(im.resize((im.width * Z, im.height * Z), Image.NEAREST), (0, y))
    y += im.height * Z + 6 * Z
    print("  %-18s %-20s %dx%d" % (os.path.basename(f), t, im.width, im.height))

c.save("check_head_yaw.png")
print("  -> check_head_yaw.png %dx%d" % c.size)
