#!/usr/bin/env python3
"""Render a mock of the eye-control UI using the GENERATED font C files.

Deliberately parses the .c output rather than reusing the generator's
in-memory data: that way the picture validates what actually gets compiled,
including glyph descriptors, the sparse cmap and the baseline arithmetic.

Layout mirrors eye_ui_design v2 on the real 800x480 panel.
"""
import re
import sys

from PIL import Image, ImageDraw

FONT_FILES = {
    "icon": "/tmp/eye_ui_fonts/eye_ui_font_48.c",
    "big": "/tmp/eye_ui_fonts/eye_ui_font_32.c",
    "small": "/tmp/eye_ui_fonts/eye_ui_font_24.c",
}

W, H = 800, 480
BG = (16, 16, 20)
FG = (235, 235, 240)
SEL_BG = (0, 140, 160)
SEL_FG = (255, 255, 255)
DIM = (150, 150, 160)
BTN_BG = (38, 38, 46)
BTN_EDGE = (72, 72, 84)
SEL_EDGE = (0, 210, 230)


def parse_font(path):
    src = open(path, encoding="utf-8").read()

    m = re.search(r"glyph_bitmap\[\] = \{(.*?)\n\};", src, re.S)
    data = bytes(int(x, 16) for x in re.findall(r"0x([0-9a-f]{2})", m.group(1)))

    m = re.search(r"glyph_dsc\[\] = \{(.*?)\n\};", src, re.S)
    dscs = []
    for g in re.finditer(
            r"\.bitmap_index = (\d+), \.adv_w = (\d+), \.box_w = (\d+), "
            r"\.box_h = (\d+), \.ofs_x = (-?\d+), \.ofs_y = (-?\d+)", m.group(1)):
        dscs.append(dict(bitmap_index=int(g.group(1)), adv_w=int(g.group(2)),
                         box_w=int(g.group(3)), box_h=int(g.group(4)),
                         ofs_x=int(g.group(5)), ofs_y=int(g.group(6))))

    m = re.search(r"unicode_list_0\[\] = \{(.*?)\};", src, re.S)
    rcps = [int(x, 16) for x in re.findall(r"0x([0-9a-f]+)", m.group(1))]

    range_start = int(re.search(r"\.range_start = (\d+)", src).group(1))
    line_height = int(re.search(r"\.line_height = (\d+)", src).group(1))
    base_line = int(re.search(r"\.base_line = (\d+)", src).group(1))

    lookup = dict((r, i + 1) for i, r in enumerate(rcps))   # glyph_id_start = 1
    return dict(data=data, dscs=dscs, lookup=lookup,
                range_start=range_start, line_height=line_height, base_line=base_line)


def draw_text(draw, font, x, line_top, text, color):
    """Replicate lv_draw_label's positioning for a single line."""
    pos = x
    missing = []
    for ch in text:
        cp = ord(ch)
        gid = font["lookup"].get(cp - font["range_start"])
        if gid is None:
            missing.append(ch)
            pos += font["line_height"] // 2
            continue
        d = font["dscs"][gid]
        if d["box_w"] == 0 or d["box_h"] == 0:
            pos += d["adv_w"] / 16.0
            continue
        y_top = line_top + (font["line_height"] - font["base_line"]) - d["box_h"] - d["ofs_y"]
        x_left = pos + d["ofs_x"]
        bpr = (d["box_w"] + 7) // 8
        for r in range(d["box_h"]):
            base = d["bitmap_index"] + r * bpr
            row = font["data"][base:base + bpr]
            for c in range(d["box_w"]):
                if row[c // 8] & (0x80 >> (c % 8)):
                    px, py = int(x_left) + c, int(y_top) + r
                    if 0 <= px < W and 0 <= py < H:
                        draw.point((px, py), fill=color)
        pos += d["adv_w"] / 16.0
    return pos, missing


def text_width(font, text):
    total = 0
    for ch in text:
        gid = font["lookup"].get(ord(ch) - font["range_start"])
        total += (font["dscs"][gid]["adv_w"] / 16.0) if gid else font["line_height"] / 2
    return total


def main():
    icon = parse_font(FONT_FILES["icon"])
    big = parse_font(FONT_FILES["big"])
    small = parse_font(FONT_FILES["small"])

    img = Image.new("RGB", (W, H), BG)
    draw = ImageDraw.Draw(img)

    missing_all = []

    # ---- title ----------------------------------------------------------
    title = "\u773c\u63a7\u9762\u677f"          # 眼控面板
    tw = text_width(big, title)
    draw_text(draw, big, (W - tw) / 2, 4, title, FG)

    # ---- menu: 2x3 grid of square buttons -------------------------------
    # Discrete grid steps rather than a long list: each look-left/look-right
    # moves exactly one cell, which is far easier to aim with gaze than a
    # scrolling list.
    # (icon, label) - the icon is a large single glyph, phone-home style.
    items = [
        ("\u773c", "\u76f8\u673a\u9884\u89c8"),        # 眼  / 相机预览
        ("\u706f", "LED \u63a7\u5236"),                # 灯  / LED 控制
        ("\u62cd", "\u62cd\u7167"),                    # 拍  / 拍照
        ("\u8baf", "\u7cfb\u7edf\u4fe1\u606f"),        # 讯  / 系统信息
        ("\u4e32", "\u4e32\u53e3\u547d\u4ee4"),        # 串  / 串口命令
        ("\u95ee", "\u5173\u4e8e"),                    # 问  / 关于
    ]
    cols, rows = 3, 2
    gap = 16
    mx, mtop, mbot = 30, 58, 46
    bw = (W - 2 * mx - (cols - 1) * gap) // cols
    bh = (H - mtop - mbot - (rows - 1) * gap) // rows

    icon_y = 26                       # icon line top, relative to cell
    label_y = 112                     # label line top, relative to cell

    for i, (glyph, label) in enumerate(items):
        r, c = divmod(i, cols)
        x0 = mx + c * (bw + gap)
        y0 = mtop + r * (bh + gap)
        x1, y1 = x0 + bw - 1, y0 + bh - 1
        sel = (i == 0)
        draw.rectangle([x0, y0, x1, y1],
                       fill=SEL_BG if sel else BTN_BG,
                       outline=SEL_EDGE if sel else BTN_EDGE)

        gw = text_width(icon, glyph)
        _, miss = draw_text(draw, icon, x0 + (bw - gw) / 2, y0 + icon_y, glyph,
                            SEL_FG if sel else FG)
        missing_all += miss

        lw = text_width(small, label)
        _, miss = draw_text(draw, small, x0 + (bw - lw) / 2, y0 + label_y, label,
                            SEL_FG if sel else DIM)
        missing_all += miss

    # selected-cell marker on the left edge, like a focus bar
    draw.rectangle([mx - 8, mtop + 8, mx - 4, mtop + bh - 9], fill=SEL_EDGE)

    # ---- hint bar -------------------------------------------------------
    draw.rectangle([0, H - 58, W - 1, H - 1], fill=(28, 28, 34))
    hint_l = "\u770b\u5de6/\u53f3 \u9009\u62e9"      # 看左/右 选择
    hint_m = "\u95ed\u773c \u786e\u8ba4"             # 闭眼 确认
    hint_r = "\u7b2c 1/6 \u9879"                     # 第 1/6 项
    _, miss = draw_text(draw, small, 40, H - 46, hint_l, DIM)
    missing_all += miss
    mw = text_width(small, hint_m)
    _, miss = draw_text(draw, small, (W - mw) / 2, H - 46, hint_m, DIM)
    missing_all += miss
    rw = text_width(small, hint_r)
    _, miss = draw_text(draw, small, W - 40 - rw, H - 46, hint_r, DIM)
    missing_all += miss

    out = "/tmp/eye_ui_fonts/ui_preview.png"
    img.save(out)

    print("\u2550" * 60)
    print("preview written: %s" % out)
    for key, f in (("icon ", icon), ("big  ", big), ("small", small)):
        print("  %s font: %d glyphs, line_height=%d base_line=%d"
              % (key, len(f["dscs"]) - 1, f["line_height"], f["base_line"]))
    if missing_all:
        print("  MISSING GLYPHS: %s" % " ".join(sorted(set(missing_all))))
    else:
        print("  all glyphs resolved")
    print("\u2550" * 60)


if __name__ == "__main__":
    main()
