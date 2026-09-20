#!/usr/bin/env python3
"""Compare the user's CubeIDE-built hex (fsbl_pinatr.hex) with our padded hex,
and validate both structures (header fields, padding, vector table, code diff,
byte_sum, presence of ConfigPinAttributes/SECCFGR writes)."""
import sys
from analyze_fsbl_header import parse_hex, u32

def main():
    user_hex = sys.argv[1] if len(sys.argv) > 1 else 'fsbl_pinatr.hex'
    my_hex = sys.argv[2] if len(sys.argv) > 2 else 'fsbl_pinatr_padded.hex'

    du, baseu = parse_hex(user_hex)   # du: bytes starting at baseu
    dm, basem = parse_hex(my_hex)

    print('=' * 70)
    print('user CubeIDE hex :', user_hex, ' base=0x%08X len=0x%X' % (baseu, len(du)))
    print('my padded hex    :', my_hex, ' base=0x%08X len=0x%X' % (basem, len(dm)))
    print('=' * 70)

    def fields(d):
        return dict(
            magic=u32(d, 0x00), bsum=u32(d, 0x64), ver=u32(d, 0x68),
            size=u32(d, 0x6C), entry=u32(d, 0x70), origin=u32(d, 0x84),
            sign_off=u32(d, 0x88), sign_sz=u32(d, 0x8C),
            msp=u32(d, 0x400), reset=u32(d, 0x404))

    fu, fm = fields(du), fields(dm)
    print('header field             user             mine')
    for k in ('magic', 'bsum', 'ver', 'size', 'entry', 'origin', 'sign_off', 'sign_sz', 'msp', 'reset'):
        print('  %-8s 0x%08X    0x%08X' % (k, fu[k], fm[k]))

    for label, d in (('user', du), ('mine', dm)):
        pad = d[0x240:0x400]
        nonzero = sum(1 for v in pad if v != 0)
        print('%s 0x240-0x400 all-zero: %s (nonzero:%d)' % (label, nonzero == 0, nonzero))

    for label, d in (('user', du), ('mine', dm)):
        size = u32(d, 0x6C)
        img = d[0x240:0x240 + size]
        bs = sum(img) & 0xFFFFFFFF
        print('%s: size=0x%X byte_sum=0x%08X vs header 0x%08X %s'
              % (label, size, bs, u32(d, 0x64),
                 'OK' if bs == u32(d, 0x64) else 'MISMATCH!'))

    # code content comparison (file 0x400+)
    cu = du[0x400:0x400 + 0x7400]
    cm = dm[0x400:0x400 + 0x7400]
    n = min(len(cu), len(cm))
    diff = sum(1 for i in range(n) if cu[i] != cm[i])
    print('code file0x400+ : user len=0x%X mine len=0x%X first0x%X diff=%d'
          % (len(du) - 0x400, len(dm) - 0x400, n, diff))
    if diff:
        offs = [i for i in range(n) if cu[i] != cm[i]][:12]
        for o in offs:
            print('  diff @ code+0x%X (file 0x%X): user=0x%02X mine=0x%02X'
                  % (o, 0x400 + o, cu[o], cm[o]))

    targets = (0x56021000, 0x56021800, 0x56021018, 0x56021818,
               0x56021030, 0x56021830, 0x56021034, 0x56021834,
               0x50021000, 0x54024000, 0x5402c000, 0x54026000, 0x54029000)
    for label, d in (('user', du), ('mine', dm)):
        hits = []
        for a in range(0x400, len(d) - 3):
            v = u32(d, a)
            if v in targets:
                hits.append((a, v))
        print('%s: base/attr consts in code: %d hits' % (label, len(hits)))
        for a, v in hits[:14]:
            print('    file 0x%X = 0x%08X' % (a, v))

if __name__ == '__main__':
    main()
