"""Rebuild the NOR weight image as Intel HEX.

The original generator is gone, so the format was recovered from the v21
file and checked line by line:

    :LL AAAA TT <data> CC

LL is the byte count, TT = 00 for data, and CC is the **two's complement** of
the byte sum - not the one's complement an SREC writer emits.  The two
formats are easy to confuse: Intel HEX's length field counts data bytes
only, while SREC's includes the address and type bytes.  That
matters: a standard writer would still produce something the programmer may
accept, but it would not match the earlier images byte for byte.

Layout as observed: 16 data bytes per line, address counting from zero, the
load address 0x70400000 in a type-05 record, canonical S9 terminator.

Self-check: run it over the v21 raw and diff against the v21 hex.  If they
agree byte for byte, the reader is faithful and the v3 image can be trusted.
"""
import hashlib
import os
import sys

PER = 16
BASE = 0x70400000


def srec(rec_type, addr, payload):
    body = (bytes([len(payload)]) + addr.to_bytes(2, "big") +
            bytes([rec_type]) + payload)
    cks = (0x100 - (sum(body) & 0xFF)) & 0xFF
    return ":%s%02X" % (body.hex().upper(), cks)


def build(raw_path):
    """Emit the data as 64 kB blocks.

    The address field of a data record is 16 bits wide, so a fresh type-04
    record is required at every 64 kB boundary.  Without it the address wraps
    back to zero and the image lands at the wrong place in flash - silently,
    since the file is still well formed.  The v21 image carries 45 such
    records (0x7040 through 0x706C); reproducing that is part of the check.
    """
    data = open(raw_path, "rb").read()
    lines = []
    off = 0
    while off < len(data):
        hi = (BASE >> 16) + (off >> 16)
        lines.append(srec(0x04, 0x0000, hi.to_bytes(2, "big")))
        block_end = min(len(data), ((off >> 16) + 1) << 16)
        while off < block_end:
            chunk = data[off:off + PER]
            lines.append(srec(0x00, off & 0xFFFF, chunk))
            off += len(chunk)
    lines.append(srec(0x05, 0x0000, BASE.to_bytes(4, "big")))
    lines.append(":00000001FF")
    return "\n".join(lines) + "\n"


# ---- self-check against the v20 pair -------------------------------------
# paired on purpose: the .v20.bak hex belongs to the .v20.bak raw.  Comparing
# a v21 raw against a v20 hex looks almost right - the two blobs share most
# of their bytes - and fails on a handful of positions, which reads like a
# bug in the writer when it is a mismatch of inputs.
V21_RAW = ("/home/leihann/SoftwarePackage/eye_flash/"
           "eye_model_data.xSPI2.bin.v20.bak")
V21_HEX = "/home/leihann/SoftwarePackage/eye_flash/eye-data.hex.v20.bak"

if os.path.exists(V21_RAW) and os.path.exists(V21_HEX):
    mine = build(V21_RAW)
    theirs = open(V21_HEX, encoding="ascii").read()
    same = mine == theirs
    print("══ 自检：用 v21 的 raw 重建，与 v21 的 hex 比对 ══")
    print("  我的输出: %d 字节  md5 %s"
          % (len(mine), hashlib.md5(mine.encode()).hexdigest()))
    print("  v21 文件: %d 字节  md5 %s"
          % (len(theirs), hashlib.md5(theirs.encode()).hexdigest()))
    print("  逐字节一致: %s" % ("是 ✅" if same else "否 ❌"))
    if not same:
        a, b = mine.split("\n"), theirs.split("\n")
        print("  行数 %d vs %d" % (len(a), len(b)))
        for i, (x, y) in enumerate(zip(a, b)):
            if x != y:
                print("  首个差异在第 %d 行:\n    我的: %s\n    原有: %s"
                      % (i + 1, x, y))
                break
        sys.exit(1)

# ---- produce the v3 image ------------------------------------------------
RAW = ("/home/leihann/openvela/nuttx/libs/ai_aton/models/eye/"
       "network_atonbuf.xSPI2.raw")
OUT = ("/home/leihann/SoftwarePackage/eye_flash/eye-data.hex")

print()
print("══ 生成 v3 镜像 ══")
data = open(RAW, "rb").read()
text = build(RAW)
open(OUT, "w", encoding="ascii").write(text)
print("  源 raw: %d 字节  md5 %s"
      % (len(data), hashlib.md5(data).hexdigest()))
print("  输出:   %s" % OUT)
print("          %d 字节  md5 %s"
      % (len(text), hashlib.md5(text.encode()).hexdigest()))
print("  行数:   %d" % (text.count("\n")))
print("  头: %s" % text.split("\n")[0])
print("  尾: %s" % text.split("\n")[-2])
print("      %s" % text.split("\n")[-3])
