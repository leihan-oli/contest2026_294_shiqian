#!/usr/bin/env python3
"""从 fsbl.bin 生成完整可烧录 FSBL hex（base=0x70000000）。

修复 make_padded_v2.py 的截断 bug：它从复用的旧 header 读 0x6C 作为
image 尺寸，导致比旧镜像大的 bin 被截断（SystemInit 等被砍掉 -> 全灭）。
本脚本用实际 bin 长度重算 0x6C/0x70/0x64。

header 模板取自已烧录验证可启动的 images/fsbl_rifsecfix.hex（本仓库自包含）。

用法: python3 gen_fsbl_hex.py <fsbl.bin> <out.hex>
"""
import os
import sys
from analyze_fsbl_header import parse_hex, u32
from make_padded_v2 import intel_hex

bin_path = sys.argv[1]
out_path = sys.argv[2]

# header 模板：本仓库已烧录验证可启动的 hex（含正确 magic/版本/origin）
_tpl = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                    '..', 'images', 'fsbl_rifsecfix.hex')
if not os.path.exists(_tpl):
    _tpl = 'fsbl_fix_padded.hex'  # 兼容旧路径

data = open(bin_path, 'rb').read()
msp = u32(data, 0)
rst = u32(data, 4)
print('bin len=0x%X  MSP=0x%08X  Reset=0x%08X' % (len(data), msp, rst))

tpl, _ = parse_hex(_tpl)
header = bytearray(tpl[:0x240])

PAD = 0x1C0
img = b'\x00' * PAD + data
new_size = len(img)  # = 0x1C0 + len(bin)，真实尺寸！

def set32(buf, off, val):
    buf[off:off + 4] = val.to_bytes(4, 'little')

set32(header, 0x6C, new_size)          # image size（真实！）
set32(header, 0x70, rst)               # Reset_Handler
set32(header, 0x64, sum(img) & 0xFFFFFFFF)  # byte_sum（全零填充不影响）

print('0x6C=0x%08X  0x70=0x%08X  0x64=0x%08X' % (new_size, rst, sum(img) & 0xFFFFFFFF))

full = bytes(header) + img
open(out_path, 'w').write(intel_hex(full, 0x70000000))
print('已写入: %s (0x%X 字节)' % (out_path, len(full)))

# ===== 回读验证 =====
d2, b2 = parse_hex(out_path)
img2 = d2[0x240:0x240 + u32(d2, 0x6C)]
print('回读: base=0x%X len=0x%X' % (b2, len(d2)))
print('  0x64=0x%08X 0x6C=0x%08X 0x70=0x%08X' % (u32(d2, 0x64), u32(d2, 0x6C), u32(d2, 0x70)))
print('  code_len=0x%X (完整应为 0x%X)' % (u32(d2, 0x6C) - 0x1C0, len(data)))
print('  byte_sum 校验:', 'OK' if (sum(img2) & 0xFFFFFFFF) == u32(d2, 0x64) else 'FAIL')
print('  0x240-0x400 全零:', all(b == 0 for b in d2[0x240:0x400]))
print('  file 0x400 MSP=0x%08X Reset=0x%08X' % (u32(d2, 0x400), u32(d2, 0x404)))
