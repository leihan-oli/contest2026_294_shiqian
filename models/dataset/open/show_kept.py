"""Look at the surviving extremes: closest, farthest, softest."""
from PIL import Image
import os

pick = ["open_0140.bmp",   # smallest dx kept (71.3)
        "open_0163.bmp",   # largest dx (114.5)
        "open_0165.bmp",   # lowest gradient
        "open_0168.bmp",   # 2nd lowest gradient
        "open_0001.bmp"]   # a typical one

pick = [f for f in pick if os.path.exists(f)]
ims = [Image.open(f).convert("RGB") for f in pick]

Z = 3
W = max(i.width for i in ims)
H = sum(i.height for i in ims) + 6 * (len(ims) - 1)

c = Image.new("RGB", (W * Z, H * Z), (40, 40, 40))
y = 0
for f, im in zip(pick, ims):
    c.paste(im.resize((im.width * Z, im.height * Z), Image.NEAREST), (0, y))
    y += im.height * Z + 6 * Z
    print("  %-16s %dx%d" % (f, im.width, im.height))

c.save("check_kept.png")
print("  -> check_kept.png %dx%d" % c.size)
