#!/usr/bin/env python3
"""Raise the crop's bottom edge by a quarter, leaving the top edge alone."""
import hashlib
import sys

P = "apps/examples/eye_cam/eye_cam_main.c"
s = open(P, encoding="utf-8").read()
before = len(s)


def sub(anchor, new, tag):
    global s
    n = s.count(anchor)
    if n != 1:
        sys.exit("FAIL %s: anchor x%d" % (tag, n))
    s = s.replace(anchor, new, 1)
    print("  ok  %s" % tag)


sub("#define EYE_CAM_CROP_K          2.0f\n"
    "#define EYE_CAM_CROP_EYE_REL    0.31f\n",
    "#define EYE_CAM_CROP_K          2.0f\n"
    "#define EYE_CAM_CROP_H_SCALE    0.75f   /* bottom edge up by a quarter */\n"
    "\n"
    "/* The eye keeps the same margin above it as before.  That margin is\n"
    " * what absorbs head movement, and an occlusion test on five captures\n"
    " * found the model barely reads it.  Raising the bottom edge while\n"
    " * holding the top one means the eye's fractional position grows by\n"
    " * the inverse of the height scale - hence 0.31 / 0.75.  The strip\n"
    " * that goes is the one the same test put at a fifth of the eyes'\n"
    " * contribution, most of that being an artefact of the mask fill.\n"
    " */\n"
    "\n"
    "#define EYE_CAM_CROP_EYE_REL    0.413f\n",
    "constants")

sub("              g_crop_h = (int)((float)g_crop_w *\n"
    "                               (float)EYE_CAM_ROI_H /\n"
    "                               (float)EYE_CAM_ROI_W + 0.5f);\n",
    "              g_crop_h = (int)((float)g_crop_w *\n"
    "                               (float)EYE_CAM_ROI_H /\n"
    "                               (float)EYE_CAM_ROI_W *\n"
    "                               EYE_CAM_CROP_H_SCALE + 0.5f);\n",
    "crop height")

sub("              /* 1.8 pupil distances wide with the eyes 0.31 of the way\n"
    "               * down: the geometry the PC side settled on, where 115 of\n"
    "               * 115 full frames cropped onto the eyes.\n"
    "               */\n",
    "              /* 2.0 pupil distances wide, bottom edge a quarter of the\n"
    "               * way up: the geometry the PC side settled on, where 115\n"
    "               * of 115 full frames cropped onto the eyes, with the\n"
    "               * below-the-eyes strip trimmed off.\n"
    "               */\n",
    "comment")

out = s
assert len(out) > before, "file shrank"
open(P, "w", encoding="utf-8").write(out)

back = open(P, encoding="utf-8").read()
assert back == out, "read-back mismatch"
for probe in ("EYE_CAM_CROP_H_SCALE    0.75f", "EYE_CAM_CROP_EYE_REL    0.413f",
              "EYE_CAM_CROP_H_SCALE + 0.5f"):
    assert probe in back, "missing: " + probe
    print("  ok  probe %s" % probe)

print("\n%d -> %d bytes, md5 %s"
      % (before, len(out), hashlib.md5(out.encode()).hexdigest()))
