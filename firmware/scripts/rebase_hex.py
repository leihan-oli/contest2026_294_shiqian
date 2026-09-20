#!/usr/bin/env python3
"""Re-base the padded FSBL hex from 0x34180000 (SRAM run address) to
0x70000000 (NOR Flash programming address).  Data content is unchanged;
only the address records in the Intel HEX are rewritten."""
import sys
from analyze_fsbl_header import parse_hex, u32
from make_padded_v2 import intel_hex

SRC = 'fsbl_pinatr_padded.hex'
OUT = 'fsbl_pinatr_padded_7000.hex'
NEW_BASE = 0x70000000

d, base = parse_hex(SRC)
print('src base=0x%08X len=0x%X' % (base, len(d)))
print('header: magic=0x%08X size=0x%X entry=0x%08X' %
      (u32(d, 0), u32(d, 0x6C), u32(d, 0x70)))
with open(OUT, 'w') as f:
    f.write(intel_hex(d, NEW_BASE))
print('wrote', OUT, 'base=0x%08X' % NEW_BASE)

# verify round-trip
d2, b2 = parse_hex(OUT)
print('verify: base=0x%08X len=0x%X size=0x%X entry=0x%08X msp=0x%08X reset=0x%08X'
      % (b2, len(d2), u32(d2, 0x6C), u32(d2, 0x70), u32(d2, 0x400), u32(d2, 0x404)))
same = d2 == d
print('data identical to source padded hex:', same)
if not same:
    diff = sum(1 for i in range(min(len(d), len(d2))) if d[i] != d2[i])
    print('diff bytes:', diff)
