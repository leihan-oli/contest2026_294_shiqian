#!/usr/bin/env python3
"""Side-by-side comparison: vendor GBK dot matrix vs Noto TTF.

Both are binarized to 1bpp (LVGL's PLAIN format), because the panel gets 1bpp
glyphs either way.  Rendering this shows whether the vendor dot data is good
enough for an eye-control panel that must be readable from a distance.
"""
from PIL import Image, ImageDraw, ImageFont

FONT_DIR = "/home/leihann/SoftwarePackage/5\uff0cSD\u5361\u6839\u76ee\u5f55\u6587\u4ef6/SYSTEM/FONT"
TTF = "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"

SIZE = 32
TEXTS = [
    "\u773c\u63a7\u9762\u677f",              # 眼控面板
    "\u76f8\u673a\u9884\u89c8",              # 相机预览
    "\u7cfb\u7edf\u4fe1\u606f",              # 系统信息
    "\u4e32\u53e3\u547d\u4ee4",              # 串口命令
]
ZOOM = 2


def gbk_index(high, low):
    return (high - 0x81) * 190 + (low - 0x40) - (1 if low > 0x7F else 0)


def fon_render(text, size, fon):
    """Return a grayscale image of text drawn from the dot-matrix font."""
    bpc = size * ((size + 7) // 8)
    w = size * len(text)
    img = Image.new("L", (w, size), 0)
    px = img.load()
    for i, ch in enumerate(text):
        code = ch.encode("gbk")
        off = gbk_index(code[0], code[1]) * bpc
        bits = fon[off:off + bpc]
        bpr = (size + 7) // 8
        for r in range(size):
            row = bits[r * bpr:(r + 1) * bpr]
            for c in range(size):
                if row[c // 8] & (0x80 >> (c % 8)):
                    px[i * size + c, r] = 255
    return img


def ttf_render(text, size, pil_font):
    """Same text, rasterized by FreeType and binarized."""
    w = int(pil_font.getlength(text)) + 4
    h = size * 2
    img = Image.new("L", (w, h), 0)
    draw = ImageDraw.Draw(img)
    draw.text((2, size + size // 2), text, font=pil_font, fill=255, anchor="ls")
    return img


def main():
    fon = open("%s/GBK%d.FON" % (FONT_DIR, SIZE), "rb").read()
    ttf = ImageFont.truetype(TTF, SIZE)

    rows = []
    for t in TEXTS:
        a = fon_render(t, SIZE, fon)
        b = ttf_render(t, SIZE, ttf)
        # trim the TTF canvas to its ink so the two rows are comparable
        bbox = b.getbbox()
        if bbox:
            b = b.crop(bbox)
        rows.append((a, b))

    pad = 12
    line_h = SIZE + pad
    W = max(max(a.width, b.width) for a, b in rows) + 220
    H = len(rows) * line_h * 2 + pad * 2 + 30
    canvas = Image.new("L", (W, H), 24)
    draw = ImageDraw.Draw(canvas)
    label_font = ImageFont.truetype(TTF, 18)

    y = 10
    draw.text((6, y), "vendor GBK  %dpx dot matrix" % SIZE, font=label_font, fill=200)
    y += 26
    for a, _ in rows:
        canvas.paste(a, (200, y))
        y += line_h

    draw.text((6, y + 6), "Noto TTF  %dpx binarized" % SIZE, font=label_font, fill=200)
    y += 34
    for _, b in rows:
        canvas.paste(b, (200, y))
        y += line_h

    out = "/tmp/eye_ui_fonts/source_compare.png"
    canvas.resize((W * ZOOM, H * ZOOM), Image.NEAREST).save(out)
    print("written: %s  (%dx%d)" % (out, W * ZOOM, H * ZOOM))


if __name__ == "__main__":
    main()
