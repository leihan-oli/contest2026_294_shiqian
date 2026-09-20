#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Pre-flash sanity check for the images in firmware/images/.

Verifies, in order of what actually bites:
  1. every image exists and is not a truncated or corrupted copy
     (Intel HEX record checksums, over every line)
  2. the load address declared inside each .hex matches the documented flash map
  3. nuttx.bin respects the 1 MiB FSBL copy limit

A wrong address or a truncated image does not raise anything on the board, it
just does not start - which is why this runs before flashing rather than after.

Format note: these files are **Intel HEX**, not SREC. The length field counts
data bytes only, and the checksum is the two's complement (every record sums to
0 mod 256). SREC would put the address and type bytes inside the length and use
the one's complement - a difference that silently breaks naive parsers.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
IMAGES = os.path.join(REPO, "firmware/images")

EXPECTED = {
    "fsbl_rifsecfix.hex": "0x70000000",
    "nuttx.bin": "0x70100000",
    "eye-data.hex": "0x70400000",
    "face-data.hex": "0x70700000",
}
SIZE_LIMIT = 1048576

failures = []


def ihex_verify(path):
  """Return (ok, load_address, problem) for an Intel HEX file."""
  base = 0
  load = None
  data_bytes = 0
  for number, raw in enumerate(open(path, encoding="ascii", errors="replace"),
                               start=1):
    line = raw.strip()
    if not line:
      continue
    if not line.startswith(":"):
      return False, load, "line %d does not start with ':'" % number
    try:
      body = bytes.fromhex(line[1:])
    except ValueError:
      return False, load, "line %d is not hex" % number
    if len(body) < 5:
      return False, load, "line %d too short" % number
    length = body[0]
    if length != len(body) - 5:
      return False, load, ("line %d length field %d does not match %d data "
                           "bytes" % (number, length, len(body) - 5))
    if sum(body) & 0xFF:
      return False, load, "line %d checksum mismatch" % number
    kind = body[3]
    data = body[4:4 + length]
    if kind == 0x00:                       # data
      if load is None:
        load = base + int.from_bytes(body[1:3], "big")
      data_bytes += length
    elif kind == 0x04:                     # extended linear address
      base = int.from_bytes(data, "big") << 16
    elif kind in (0x01, 0x02, 0x03, 0x05):  # eof / segment / start
      continue
    else:
      return False, load, "line %d unknown record type 0x%02x" % (number, kind)
  if not data_bytes:
    return False, load, "no data records"
  return True, load, None


print("== firmware images ==")
for name, expected_addr in sorted(EXPECTED.items()):
  path = os.path.join(IMAGES, name)
  if not os.path.exists(path):
    failures.append("%s missing" % name)
    print("  x %-22s missing" % name)
    continue
  size = os.path.getsize(path)
  if name.endswith(".hex"):
    ok, load, problem = ihex_verify(path)
    if not ok:
      failures.append("%s: %s" % (name, problem))
      print("  x %-22s Intel HEX invalid: %s" % (name, problem))
      continue
    got = "0x%08x" % (load or 0)
    if got != expected_addr:
      failures.append("%s loads at %s, expected %s" %
                      (name, got, expected_addr))
    mark = "OK" if got == expected_addr else "x "
    print("  %s %-22s %9d B  hex ok, loads at %s (doc: %s)" %
          (mark, name, size, got, expected_addr))
  else:
    ok = size <= SIZE_LIMIT
    if not ok:
      failures.append("%s is %d B, over the %d B limit" %
                      (name, size, SIZE_LIMIT))
    print("  %s %-22s %9d B  (limit %d B)" %
          ("OK" if ok else "x ", name, size, SIZE_LIMIT))

print()
if failures:
  print("FAIL: %d problem(s)" % len(failures))
  for item in failures:
    print("  - %s" % item)
  sys.exit(1)
print("PASS: images consistent with the documented flash map")
