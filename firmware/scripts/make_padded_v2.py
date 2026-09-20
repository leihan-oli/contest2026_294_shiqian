#!/usr/bin/env python3
"""通用：从任意未填充 FSBL hex 生成带 0x1C0 填充的正确结构 hex。

自动检测向量表位置和真正的 Reset_Handler，修正 header 0x70/0x6C。
0x64 (byte_sum) 全零填充不变，自动保持。
用法: python3 make_padded_v2.py <src.hex> [out.hex]
"""
import sys
from analyze_fsbl_header import parse_hex, u32

def set32(buf, off, val):
    buf[off:off + 4] = val.to_bytes(4, 'little')

def find_vector_table(img, start=0x000, end=0x600):
    """在 image 中找向量表: 返回 (image_off, MSP, Reset)"""
    for off in range(start, end, 4):
        msp = u32(img, off)
        rst = u32(img, off + 4)
        if msp == 0x34200000 and (rst & 0xFFFF0000) == 0x34180000:
            return off, msp, rst
    return None, None, None

def intel_hex(data, base_addr, rec_len=16):
    lines = []
    chunk_base = None
    addr = base_addr
    pos = 0
    while pos < len(data):
        if (addr & 0xFFFF0000) != chunk_base:
            chunk_base = addr & 0xFFFF0000
            ela = '%04X' % ((chunk_base >> 16) & 0xFFFF)
            payload = bytes.fromhex('02000004' + ela)
            ck = (~sum(payload) + 1) & 0xFF
            lines.append(':02000004%s%02X' % (ela, ck))
        blen = min(rec_len, len(data) - pos)
        body = data[pos:pos + blen]
        rec = '%02X%04X00' % (blen, addr & 0xFFFF)
        ck = (~sum(bytes.fromhex(rec + body.hex())) + 1) & 0xFF
        lines.append(':%s%s%02X' % (rec, body.hex().upper(), ck))
        pos += blen
        addr += blen
    lines.append(':00000001FF')
    return '\n'.join(lines) + '\n'

def main():
    src = sys.argv[1] if len(sys.argv) > 1 else 'fsbl_new.hex'
    out = sys.argv[2] if len(sys.argv) > 2 else src.replace('.hex', '_padded.hex')

    d, base = parse_hex(src)
    print('源: %s base=0x%X len=0x%X' % (src, base, len(d)))
    print('header: 0x64=0x%08X 0x6C=0x%08X 0x70=0x%08X' %
          (u32(d, 0x64), u32(d, 0x6C), u32(d, 0x70)))

    header = bytearray(d[:0x240])
    img_size = u32(d, 0x6C)
    img = d[0x240:0x240 + img_size]

    # 检测向量表位置
    vt_off, msp, rst = find_vector_table(img)
    if vt_off is None:
        print('ERROR: 未找到向量表')
        sys.exit(1)
    print('向量表 @ image 0x%X: MSP=0x%08X Reset=0x%08X' % (vt_off, msp, rst))

    if vt_off == 0x1C0:
        print('✅ 源已是填充结构（向量表在 image 0x1C0），无需处理')
        open(out, 'w').write(intel_hex(d, base))
        return

    # 构造: [header 0x240][0x1C0 zeros][image]
    PAD = 0x1C0
    new_img = b'\x00' * PAD + img
    new_size = len(new_img)
    print('新 image: 0x%X 字节 (填充 0x%X)' % (new_size, PAD))

    set32(header, 0x6C, new_size)          # image size
    set32(header, 0x70, rst)               # 真正的 Reset_Handler
    bs = sum(new_img) & 0xFFFFFFFF          # 全零填充不影响 byte_sum
    set32(header, 0x64, bs)
    print('修正: 0x6C=0x%08X 0x70=0x%08X 0x64=0x%08X' %
          (new_size, rst, bs))

    # 验证新结构中 BootROM 读取的位置
    print('验证: file 0x400(MSP)=0x%08X file 0x404(Reset)=0x%08X' %
          (u32(new_img, PAD), u32(new_img, PAD + 4)))

    full = bytes(header) + new_img
    open(out, 'w').write(intel_hex(full, base))
    print('已写入: %s (0x%X 字节)' % (out, len(full)))

    # 回读验证
    d2, b2 = parse_hex(out)
    print('回读: base=0x%X len=0x%X' % (b2, len(d2)))
    print('  0x64=0x%08X 0x6C=0x%08X 0x70=0x%08X' % (u32(d2, 0x64), u32(d2, 0x6C), u32(d2, 0x70)))
    print('  file 0x400=0x%08X file 0x404=0x%08X' % (u32(d2, 0x400), u32(d2, 0x404)))
    print('  0x240-0x400 全零:', all(b == 0 for b in d2[0x240:0x400]))
    img2 = d2[0x240:0x240 + u32(d2, 0x6C)]
    print('  byte_sum(image)=0x%08X (vs 0x64)' % (sum(img2) & 0xFFFFFFFF))

if __name__ == '__main__':
    main()
