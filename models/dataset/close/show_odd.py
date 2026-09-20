"""The five frames the model reads as open, beside two it reads as closed."""
from PIL import Image

ODD = ["closed_0045.bmp", "closed_0046.bmp", "closed_0075.bmp",
       "closed_0109.bmp", "closed_0211.bmp"]
REF = ["closed_0001.bmp", "closed_0002.bmp"]

pick = [f for f in ODD + REF]
ims = [Image.open(f).convert("RGB") for f in pick]

Z = 3
W = max(i.width for i in ims)
H = sum(i.height for i in ims) + 6 * (len(ims) - 1)

c = Image.new("RGB", (W * Z, H * Z), (40, 40, 40))
y = 0
for i, (f, im) in enumerate(zip(pick, ims)):
    c.paste(im.resize((im.width * Z, im.height * Z), Image.NEAREST), (0, y))
    y += im.height * Z + 6 * Z
    tag = "  <- 判 open" if i < len(ODD) else "  (判 closed)"
    print("  %-18s %dx%d%s" % (f, im.width, im.height, tag))

c.save("check_open_verdicts.png")
print("  -> check_open_verdicts.png %dx%d" % c.size)
