#!/usr/bin/env python3
"""Point the board's quantisation constants at the v3 model.

These six numbers describe how the camera pixels map into the int8 tensor and
how the raw output maps back to logits.  They come from the generated
network.c (cross-checked against the ONNX Q/DQ nodes, which agreed exactly).
A mismatch here is silent: the model still runs and still produces a verdict,
just a wrong one.

The Q_GAIN / Q_BIAS pair is the affine folding of the input path,
    v/255 -> (x - 0.5)/0.5 = v/127.5 - 1, then /scale + zero_point
so
    gain = 1 / (127.5 * scale)
    bias = zero_point - 1 / scale
Recomputing them from the new scale/zp rather than copying a number by hand.
"""
import hashlib
import sys

P = "apps/examples/eye_cam/eye_cam_main.c"
s = open(P, encoding="utf-8").read()
before = len(s)

# derived, so they cannot drift from the pair above
IS, IZ = 0.00707420241087675, -14
GAIN = 1.0 / (127.5 * IS)
BIAS = IZ - 1.0 / IS

# verify the folding against the v2 pair the board has been running
g2 = 1.0 / (127.5 * 0.007135717)
b2 = -13 - 1.0 / 0.007135717
assert abs(g2 - 1.0991379) < 1e-6, "folding check failed (gain)"
assert abs(b2 - (-153.14008)) < 1e-3, "folding check failed (bias)"
print("  ok  折叠公式自检通过（v2 反推 = 板端现值）")
print("      v3 gain = %.7f   bias = %.5f" % (GAIN, BIAS))


def sub(anchor, new, tag):
    global s
    n = s.count(anchor)
    if n != 1:
        sys.exit("FAIL %s: anchor x%d" % (tag, n))
    s = s.replace(anchor, new, 1)
    print("  ok  %s" % tag)


sub("#define EYE_CAM_IN_SCALE        0.007135717f    /* \u4e94\u5206\u7c7b"
    "\u4ea7\u7269 2026-09-18 */\n"
    "#define EYE_CAM_IN_ZP           (-13)\n"
    "#define EYE_CAM_OUT_SCALE       0.129406050f   /* \u4e94\u5206\u7c7b"
    "\u4ea7\u7269 2026-09-18 */\n"
    "#define EYE_CAM_OUT_ZP          (-38)\n",
    "#define EYE_CAM_IN_SCALE        0.0070742024f   /* v3 \u91cd\u8bad "
    "2026-09-19 */\n"
    "#define EYE_CAM_IN_ZP           (-14)\n"
    "#define EYE_CAM_OUT_SCALE       0.17015758f     /* v3 \u91cd\u8bad "
    "2026-09-19 */\n"
    "#define EYE_CAM_OUT_ZP          (-34)\n",
    "scale/zp")

sub("#define EYE_CAM_Q_GAIN          (1.0991379f)\n"
    "#define EYE_CAM_Q_BIAS          (-153.14008f)\n",
    "#define EYE_CAM_Q_GAIN          (%ff)\n" % GAIN +
    "#define EYE_CAM_Q_BIAS          (%ff)\n" % BIAS,
    "Q_GAIN/Q_BIAS")

out = s
assert len(out) > before, "file shrank"
open(P, "w", encoding="utf-8").write(out)

back = open(P, encoding="utf-8").read()
assert back == out, "read-back mismatch"

must_have = ("0.0070742024f", "(-14)", "0.17015758f", "(-34)",
             "%ff" % GAIN, "%ff" % BIAS)
must_not = ("0.007135717f", "(-13)", "0.129406050f", "(-38)", "1.0991379f")
for p in must_have:
    assert p in back, "missing new value: " + p
for p in must_not:
    assert p not in back, "old value still present: " + p

print("  ok  六个新值全部就位，旧值已清除")
print("\n%d -> %d bytes, md5 %s"
      % (before, len(out), hashlib.md5(out.encode()).hexdigest()))
