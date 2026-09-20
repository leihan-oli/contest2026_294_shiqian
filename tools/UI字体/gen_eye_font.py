#!/usr/bin/env python3
"""Generate LVGL v9 font C files for the eye-control UI.

Every glyph - Chinese and ASCII - is rasterized from Noto Sans CJK.  The
vendor GBK dot matrix was evaluated first and rejected: at 32 px a complex
ideograph gets 1 px strokes and smears into an unreadable blob (rerun
compare_sources.py to see the side-by-side).

Metrics come straight from the face:
        line_height = ascent + descent
        base_line   = descent
and each glyph is trimmed to its ink with

        ofs_y = baseline_y - ink_bottom

Matching LVGL's own formula
        glyph_top = line_top + (line_height - base_line) - box_h - ofs_y
this puts a glyph's ink bottom exactly on the baseline, so Chinese and ASCII
align inside one label.
"""
import os

from PIL import Image, ImageDraw, ImageFont

TTF = "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"
OUT_DIR = "/tmp/eye_ui_fonts"

# Every Chinese character the UI can display.  Keep this list explicit: the
# font is baked into flash, so an unknown glyph shows as nothing at all.
CJK = (
    # title + menu
    "\u773c\u63a7\u9762\u677f"          # 眼控面板
    "\u76f8\u673a\u9884\u89c8"          # 相机预览
    "\u5236\u62cd\u7167"                # 制拍照
    "\u7cfb\u7edf\u4fe1\u606f"          # 系统信息
    "\u4e32\u53e3\u547d\u4ee4"          # 串口命令
    "\u5173\u4e8e"                      # 关于
    # hints
    "\u770b\u5de6\u53f3\u9009\u62e9"    # 看左右选择
    "\u95ed\u786e\u8ba4"                # 闭确认
    "\u79d2\u81ea\u8fd4\u56de\u83dc\u5355"  # 秒自返回菜单
    "\u7b2c\u9879"                      # 第项
    # LED page
    "\u4eae\u706d\u5168\u90e8\u7184"    # 亮灭全部熄
    # photo + save
    "\u5df2\u4fdd\u5b58\u5f20\u5269"    # 已保存张剩
    # system info
    "\u5e27\u7387\u6982"                # 帧率概
    "\u7f6e\u5ea6\u5806\u4f59\u91cf"    # 置度堆余量
    "\u5f53\u524d\u6a21\u5f0f\u7248\u672c"  # 当前模式版本
    # status / errors
    "\u52a0\u8f7d\u5931\u8d25\u6210\u529f"  # 加载失败成功
    "\u9519\u8bef\u7b49\u5f85\u8bf7\u7a0d\u540e\u91cd\u8bd5"  # 错误等待请稍后重试
    "\u65e0\u6cd5\u627e\u5230"          # 无法找到
    "\u5361\u672a\u63d2\u5165\u6302\u8f7d\u4e2d"  # 卡未插入挂载中
    # generic
    "\u5f00\u6587\u6570\u5b57\u65f6\u95f4"  # 开文数字时间
    "\u65e5\u671f\u5927\u5c0f"          # 日期大小
    "\u7528\u5b9c\u5206"                # 用宜分
    # runtime states
    "\u5728\u53d6\u6d88\u6b63\u5019\u7acb\u5373\u5b8c"  # 在取消正候立即完
    "\u663e\u793a\u6253\u901a\u5b9a\u9000"  # 示打通定退
    "\u52a8\u4f5c\u53cd\u9988\u6570\u636e"  # 动作反馈数据
    "\u8f93\u51fa\u8f93\u5165\u72b6\u6001"  # 输出输入状态
    # grid icons (48 px layer)
    "\u706f\u95ee\u8baf"                # 灯问讯
    "\u89c6\u4eae\u5c4f\u56fe"          # 视亮屏图
    "\u5e2e\u52a9\u8bf4\u53f7"          # 帮助说号
)

SPECS = [
    dict(size=48, out="eye_ui_font_48.c", sym="eye_ui_font_48"),
    dict(size=32, out="eye_ui_font_32.c", sym="eye_ui_font_32"),
    dict(size=24, out="eye_ui_font_24.c", sym="eye_ui_font_24"),
]


def ink_box(bitmap, width, height):
    """Bounding box of inked pixels in a 1bpp row-major MSB-first bitmap."""
    bpr = (width + 7) // 8
    x0, y0, x1, y1 = width, height, -1, -1
    for r in range(height):
        row = bitmap[r * bpr:(r + 1) * bpr]
        for byte_i, byte in enumerate(row):
            if not byte:
                continue
            for bit in range(8):
                if byte & (0x80 >> bit):
                    c = byte_i * 8 + bit
                    if c >= width:
                        continue
                    x0 = min(x0, c)
                    x1 = max(x1, c)
                    y0 = min(y0, r)
                    y1 = max(y1, r)
    if x1 < 0:
        return None
    return (x0, y0, x1, y1)


def crop(bitmap, width, box):
    """Extract the inked rectangle as 1bpp row-major MSB-first bytes."""
    x0, y0, x1, y1 = box
    box_w = x1 - x0 + 1
    box_h = y1 - y0 + 1
    out = bytearray()
    bpr_in = (width + 7) // 8
    for r in range(y0, y1 + 1):
        row = bitmap[r * bpr_in:(r + 1) * bpr_in]
        bits = bytearray((box_w + 7) // 8)
        for c in range(x0, x1 + 1):
            byte_i, bit_i = c // 8, c % 8
            if byte_i < len(row) and (row[byte_i] & (0x80 >> bit_i)):
                cc = c - x0
                bits[cc // 8] |= 0x80 >> (cc % 8)
        out.extend(bits)
    return out, box_w, box_h


def pack_1bpp(img, width, height):
    """Threshold an 8-bit grayscale image into 1bpp, MSB first.

    ink_box()/crop() assume this layout, so the TTF branch has to convert
    before touching them - feeding them PIL's raw grayscale bytes silently
    finds no ink at all, because the byte-per-row stride is wrong.
    """
    px = img.load()
    bpr = (width + 7) // 8
    buf = bytearray(bpr * height)
    for r in range(height):
        base = r * bpr
        for c in range(width):
            if px[c, r] > 127:
                buf[base + c // 8] |= 0x80 >> (c % 8)
    return bytes(buf)


def rasterize(ch, size, pil_font, ascent):
    """Rasterize one character onto the shared baseline."""
    descent = pil_font.getmetrics()[1]
    pad = 4
    W = int(pil_font.getlength(ch)) + 2 * pad + size
    H = ascent + descent + 2 * pad
    baseline_y = ascent + pad

    img = Image.new("L", (W, H), 0)
    draw = ImageDraw.Draw(img)
    draw.text((pad, baseline_y), ch, font=pil_font, fill=255, anchor="ls")

    raw = pack_1bpp(img, W, H)
    box = ink_box(raw, W, H)
    if box is None:
        # Blank glyph such as space.  Keep an entry anyway: LVGL advances by
        # adv_w, so dropping it entirely would silently eat the space in
        # strings like "LED 控制".
        return dict(bits=b"", box_w=0, box_h=0, ofs_x=0, ofs_y=0,
                    adv_w=max(1, int(round(pil_font.getlength(ch) * 16))))

    x0, y0, x1, y1 = box
    bits, box_w, box_h = crop(raw, W, box)
    ofs_x = x0 - pad
    ofs_y = baseline_y - y1            # same semantic as the CJK branch
    adv_w = int(round(pil_font.getlength(ch) * 16))
    return dict(bits=bits, box_w=box_w, box_h=box_h,
                ofs_x=ofs_x, ofs_y=ofs_y, adv_w=adv_w)


def build_font(spec, all_codes):
    size = spec["size"]
    pil_font = ImageFont.truetype(TTF, size)
    ascent, descent = pil_font.getmetrics()

    glyphs = []                        # (codepoint, dict) in glyph_id order
    skipped = []

    for cp in all_codes:
        g = rasterize(chr(cp), size, pil_font, ascent)
        if g is None:
            skipped.append(chr(cp))
            continue
        glyphs.append((cp, g))

    glyphs.sort(key=lambda kv: kv[0])  # cmap needs ascending codepoints

    # ---- assemble the C file ------------------------------------------------
    L = []
    L.append("/*******************************************************************************")
    L.append(" * Eye-control UI font, %d px, 1 bpp (generated - do not edit by hand)" % size)
    L.append(" *")
    L.append(" * Glyphs : Noto Sans CJK Regular @ %d px, binarized to 1 bpp." % size)
    L.append(" * Metrics: line_height = %d, base_line = %d (read from the face)."
             % (ascent + descent, descent))
    L.append(" * Tool   : gen_eye_font.py")
    L.append(" ******************************************************************************/")
    L.append("")
    L.append("#ifdef LV_LVGL_H_INCLUDE_SIMPLE")
    L.append("    #include \"lvgl.h\"")
    L.append("#else")
    L.append("    #include \"lvgl/lvgl.h\"")
    L.append("#endif")
    L.append("")
    L.append("/*-----------------")
    L.append(" *    BITMAPS")
    L.append(" *----------------*/")
    L.append("")
    L.append("static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {")

    offset = 0
    dsc_rows = []
    for cp, g in glyphs:
        name = chr(cp)
        label = name if 0x20 < cp < 0x7F else "U+%04X" % cp
        if cp == 0x22:
            label = "\\\""
        L.append("    /* %s */" % label)
        line = "    "
        for i, b in enumerate(g["bits"]):
            line += "0x%02x," % b
            if (i + 1) % 16 == 0:
                L.append(line)
                line = "    "
        if line.strip():
            L.append(line)
        L.append("")
        dsc_rows.append((offset, g))
        offset += len(g["bits"])

    L.append("};")
    L.append("")
    L.append("/*-----------------")
    L.append(" *    GLYPH DESCRIPTORS")
    L.append(" *----------------*/")
    L.append("")
    L.append("static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {")
    L.append("    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,")
    for off, g in dsc_rows:
        L.append("    {.bitmap_index = %d, .adv_w = %d, .box_w = %d, .box_h = %d, .ofs_x = %d, .ofs_y = %d},"
                 % (off, g["adv_w"], g["box_w"], g["box_h"], g["ofs_x"], g["ofs_y"]))
    L.append("};")
    L.append("")
    L.append("/*-----------------")
    L.append(" *    CHARACTER MAPPING")
    L.append(" *----------------*/")
    L.append("")

    range_start = glyphs[0][0]
    rcp = [cp - range_start for cp, _ in glyphs]
    L.append("/* Codepoints relative to range_start, ascending (sparse cmap). */")
    L.append("static const uint16_t unicode_list_0[] = {")
    for i in range(0, len(rcp), 10):
        L.append("    " + ", ".join("0x%04x" % v for v in rcp[i:i + 10]) + ",")
    L.append("};")
    L.append("")
    L.append("static const lv_font_fmt_txt_cmap_t cmaps[] = {")
    L.append("    {")
    L.append("        .range_start = %d, .range_length = %d, .glyph_id_start = 1,"
             % (range_start, glyphs[-1][0] - range_start + 1))
    L.append("        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL,")
    L.append("        .list_length = %d, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY" % len(rcp))
    L.append("    }")
    L.append("};")
    L.append("")
    L.append("/*-----------------")
    L.append(" *  ALL CUSTOM DATA")
    L.append(" *----------------*/")
    L.append("")
    L.append("static const lv_font_fmt_txt_dsc_t font_dsc = {")
    L.append("    .glyph_bitmap = glyph_bitmap,")
    L.append("    .glyph_dsc = glyph_dsc,")
    L.append("    .cmaps = cmaps,")
    L.append("    .kern_dsc = NULL,")
    L.append("    .kern_scale = 0,")
    L.append("    .cmap_num = 1,")
    L.append("    .bpp = 1,")
    L.append("    .kern_classes = 0,")
    L.append("    .bitmap_format = 0,")
    L.append("};")
    L.append("")
    L.append("/*-----------------")
    L.append(" *  PUBLIC FONT")
    L.append(" *----------------*/")
    L.append("")
    L.append("const lv_font_t %s = {" % spec["sym"])
    L.append("    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,")
    L.append("    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,")
    L.append("    .line_height = %d," % (ascent + descent))
    L.append("    .base_line = %d," % descent)
    L.append("    .subpx = LV_FONT_SUBPX_NONE,")
    L.append("    .underline_position = -2,")
    L.append("    .underline_thickness = 2,")
    L.append("    .dsc = &font_dsc")
    L.append("};")
    L.append("")

    return "\n".join(L), glyphs, skipped, offset


def preview(glyphs, size):
    """ASCII-art render of a few glyphs to eyeball the extraction."""
    want = ["\u773c", "\u63a7", "\u7167", "A", "g", "5"]
    by_cp = dict((cp, g) for cp, g in glyphs)
    for w in want:
        g = by_cp.get(ord(w))
        print("\n--- '%s' %dx%d ofs(%d,%d) adv=%d ---"
              % (w, g["box_w"], g["box_h"], g["ofs_x"], g["ofs_y"], g["adv_w"]))
        bpr = (g["box_w"] + 7) // 8
        for r in range(g["box_h"]):
            row = g["bits"][r * bpr:(r + 1) * bpr]
            line = ""
            for c in range(g["box_w"]):
                line += "#" if (row[c // 8] & (0x80 >> (c % 8))) else "."
            print("    " + line)


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    all_codes = sorted(set([ord(c) for c in CJK] + list(range(0x20, 0x7F))))

    for spec in SPECS:
        text, glyphs, skipped, total = build_font(spec, all_codes)
        path = os.path.join(OUT_DIR, spec["out"])
        with open(path, "w", encoding="utf-8") as f:
            f.write(text)
        print("=" * 68)
        print("%s" % spec["out"])
        print("  glyphs   : %d" % len(glyphs))
        print("  bitmap   : %d bytes (%.1f KB)" % (total, total / 1024.0))
        print("  file     : %.1f KB" % (os.path.getsize(path) / 1024.0))
        if skipped:
            print("  SKIPPED  : %s" % "".join(skipped))
        print("  codepoint range 0x%04X .. 0x%04X" % (glyphs[0][0], glyphs[-1][0]))
        preview(glyphs, spec["size"])


if __name__ == "__main__":
    main()
