#!/usr/bin/env python3
"""Build the final padded FSBL hex from the freshly linked elf bin.

Flow:
  1. read the verified header (first 0x240 bytes) from fsbl_rif_padded.hex
  2. append the new code image (from fsbl_build/fsbl.bin, starts at 0x34180400)
  3. write a source hex, then run make_padded_v2.py to add 0x1C0 padding and
     fix header 0x64/0x6C/0x70.
"""
import sys
from analyze_fsbl_header import parse_hex
from make_padded_v2 import intel_hex

SRC = 'fsbl_rif_padded.hex'      # verified header source
BIN = 'fsbl_build/fsbl.bin'      # freshly linked code image
SRC_HEX = 'fsbl_build/fsbl_src.hex'
OUT = 'fsbl_build/fsbl_rif_fixed_padded.hex'

d, base = parse_hex(SRC)
header = bytes(d[0:0x240])
with open(BIN, 'rb') as f:
    code = f.read()
print('header:', hex(len(header)), 'bytes; bin:', hex(len(code)), 'bytes')
full = header + code
with open(SRC_HEX, 'w') as f:
    f.write(intel_hex(full, 0x34180000))
print('wrote', SRC_HEX, 'total', hex(len(full)))
