#!/usr/bin/env python3
"""FSBL STM32 boot header 分析器（修复 0x70000000 基址处理）"""
import zlib, sys

def parse_hex(path):
    """解析 Intel HEX，返回 (data, base_addr)。data 以 base_addr 为偏移起点。"""
    data = bytearray()
    base = 0          # 当前 extended linear address
    seg = 0
    min_addr = None
    max_addr = 0
    for ln in open(path):
        ln = ln.strip()
        if not ln.startswith(':'):
            continue
        t = ln[7:9]
        if t == '04':                      # Extended Linear Address
            base = int(ln[9:13], 16) << 16
            continue
        if t == '02':                      # Extended Segment Address
            seg = int(ln[9:13], 16) << 4
            continue
        if t == '01':                      # EOF
            continue
        if t != '00':                      # 忽略其他记录类型
            continue
        blen = int(ln[1:3], 16)
        addr = base + seg + int(ln[3:7], 16)
        payload = bytes.fromhex(ln[9:9 + blen * 2])
        if min_addr is None:
            min_addr = addr
        min_addr = min(min_addr, addr)
        max_addr = max(max_addr, addr + blen)
        if len(data) < max_addr - min_addr:
            data.extend(b'\xff' * (max_addr - min_addr - len(data)))
        data[addr - min_addr:addr - min_addr + blen] = payload
    return bytes(data), min_addr

def u32(b, off):
    return int.from_bytes(b[off:off + 4], 'little')

def main():
    led_path = sys.argv[1] if len(sys.argv) > 1 else 'fsbl.hex'
    pre_path = sys.argv[2] if len(sys.argv) > 2 else \
        'first/FSBL/MX25UM25645G_W958D8NBYA5I_Example/Binary/fsbl.hex'

    d_led, base_led = parse_hex(led_path)
    d_pre, base_pre = parse_hex(pre_path)
    print("LED 版 base=0x%X len=0x%X" % (base_led, len(d_led)))
    print("预编译 base=0x%X len=0x%X" % (base_pre, len(d_pre)))

    img_led = d_led
    img_pre = d_pre

    print("\n########## 1. header 字段对比 ##########")
    fields = [0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x1C,
              0x20, 0x24, 0x28, 0x2C, 0x30, 0x34, 0x38, 0x3C,
              0x40, 0x44, 0x48, 0x4C, 0x50, 0x54, 0x58, 0x5C,
              0x60, 0x64, 0x68, 0x6C, 0x70, 0x74, 0x78, 0x7C,
              0x80, 0x84, 0x88, 0x8C, 0x90, 0x94, 0x98, 0x9C]
    print("%-8s %-12s %-12s %s" % ("offset", "LED", "prebuild", "diff?"))
    for off in fields:
        v1 = u32(img_led, off)
        v2 = u32(img_pre, off)
        mark = "  <-- DIFF" if v1 != v2 else ""
        print("0x%02X   0x%08X 0x%08X%s" % (off, v1, v2, mark))

    print("\n########## 2. 向量表位置探测 ##########")
    for cand in [0x1C0, 0x200, 0x240, 0x400, 0x1C4, 0x404]:
        if cand + 8 <= len(img_led):
            msp = u32(img_led, cand)
            rst = u32(img_led, cand + 4)
            print("LED    @0x%03X: MSP=0x%08X Reset=0x%08X" % (cand, msp, rst))
        if cand + 8 <= len(img_pre):
            msp = u32(img_pre, cand)
            rst = u32(img_pre, cand + 4)
            print("pre    @0x%03X: MSP=0x%08X Reset=0x%08X" % (cand, msp, rst))

    def find_vt(b):
        for off in range(0x100, 0x800, 4):
            msp = u32(b, off)
            rst = u32(b, off + 4)
            if (msp & 0xFFFFF000) == 0x34200000 and (rst & 0xFFFF0000) == 0x34180000:
                return off, msp, rst
        return None, None, None
    vt_led, msp_led, rst_led = find_vt(img_led)
    vt_pre, msp_pre, rst_pre = find_vt(img_pre)
    print("LED 向量表 @0x%X: MSP=0x%08X Reset=0x%08X" % (vt_led, msp_led, rst_led))
    print("pre 向量表 @0x%X: MSP=0x%08X Reset=0x%08X" % (vt_pre, msp_pre, rst_pre))

    print("\n########## 3. 0x64 是否 CRC32？ ##########")
    val64_led = u32(img_led, 0x64)
    val64_pre = u32(img_pre, 0x64)
    print("LED 0x64 = 0x%08X, pre 0x64 = 0x%08X" % (val64_led, val64_pre))
    img_size_led = u32(img_led, 0x6C)
    img_size_pre = u32(img_pre, 0x6C)
    print("LED 0x6C(image size) = 0x%X (%d), pre = 0x%X (%d)" %
          (img_size_led, img_size_led, img_size_pre, img_size_pre))

    ranges = {}
    if vt_led:
        ranges['led_vt_to_end'] = img_led[vt_led:]
        ranges['led_vt_to_vt+size'] = img_led[vt_led:vt_led + img_size_led]
    if vt_pre:
        ranges['pre_vt_to_end'] = img_pre[vt_pre:]
        ranges['pre_vt_to_vt+size'] = img_pre[vt_pre:vt_pre + img_size_pre]
    if vt_led:
        ranges['led_header_to_vt'] = img_led[:vt_led]
    if vt_pre:
        ranges['pre_header_to_vt'] = img_pre[:vt_pre]
    ranges['led_whole'] = img_led
    ranges['pre_whole'] = img_pre
    for name, data in ranges.items():
        c1 = zlib.crc32(data) & 0xFFFFFFFF
        m1 = "  <== LED 0x64 MATCH" if c1 == val64_led else ""
        m2 = "  <== pre 0x64 MATCH" if c1 == val64_pre else ""
        print("%-24s crc32=0x%08X%s%s" % (name, c1, m1, m2))

    print("\n########## 4. 入口地址代码定位 ##########")
    for tag, rst, img in [("LED", rst_led, img_led), ("pre", rst_pre, img_pre)]:
        if not rst:
            continue
        found = False
        for base_guess in [0x34180000, 0x34180400, 0x34000000, 0x34100000, 0x34180000]:
            off = rst - base_guess
            if 0 <= off < len(img) - 12:
                print("%s 入口 0x%08X: 假设链接基址 0x%08X -> image off 0x%X" %
                      (tag, rst, base_guess, off))
                print("  bytes: %s" % img[off:off + 12].hex())
                found = True
                break
        if not found:
            print("%s 入口 0x%08X: 未在 image 中找到（长度 0x%X）" % (tag, rst, len(img)))

    print("\n########## 5. header 区逐字节 diff（0x00-0xA0）##########")
    ndiff = 0
    for off in range(0x00, 0xA0):
        if off < len(img_led) and off < len(img_pre):
            if img_led[off] != img_pre[off]:
                ndiff += 1
                print("  0x%02X: LED=0x%02X pre=0x%02X" % (off, img_led[off], img_pre[off]))
    print("header 0x00-0xA0 共 %d 个字节不同" % ndiff)

if __name__ == '__main__':
    main()
