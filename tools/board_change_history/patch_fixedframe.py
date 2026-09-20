#!/usr/bin/env python3
"""Fixed-frame capture, for the 'other' class.

The other class is meant to cover what the application sees when there is no
eye in view - a scene, a wall, a face turned away.  The detector rejects
every one of those, and the capture path only writes frames the detector
placed (a crop left over from an earlier frame does not describe the current
one), so none of them could be recorded at all.

This adds a mode that pins the crop instead of deriving it: centred, and at
the same size the face crops come out at, so the training set stays one
scale and the resize to the model's tensor does the same thing to every
class.

    eye_cam capture other 220 fixed          uses 190x70
    eye_cam capture other 220 fixed 150 56   any size, for other distances
"""
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


# ------------------------------------------------------------ 1. constants

sub("#define EYE_CAM_CAP_DEF_N 220\n",
    "#define EYE_CAM_CAP_DEF_N 220\n"
    "\n"
    "/* Fixed-frame captures use the size a face crop comes out at around a\n"
    " * typical working distance (dx 95 -> 190x70), so every class lands on\n"
    " * the same scale once resized to the tensor.\n"
    " */\n"
    "\n"
    "#define EYE_CAM_CAP_FIXED_W 190\n"
    "#define EYE_CAM_CAP_FIXED_H 70\n",
    "constants")

# -------------------------------------------------------------- 2. state

sub("static FAR FILE       *g_cap_meta;    /* meta.txt, append            */\n",
    "static FAR FILE       *g_cap_meta;    /* meta.txt, append            */\n"
    "static int             g_cap_fixed;   /* pin the crop, skip the face */\n"
    "static int             g_cap_fixed_w = EYE_CAM_CAP_FIXED_W;\n"
    "static int             g_cap_fixed_h = EYE_CAM_CAP_FIXED_H;\n",
    "state")

# -------------------------------------------------------- 3. argument parse

sub("      g_cap_label = (argc > 2) ? argv[2] : \"sample\";\n"
    "      g_cap_count = (argc > 3) ? atoi(argv[3]) : EYE_CAM_CAP_DEF_N;\n"
    "    }\n",
    "      g_cap_label = (argc > 2) ? argv[2] : \"sample\";\n"
    "      g_cap_count = (argc > 3) ? atoi(argv[3]) : EYE_CAM_CAP_DEF_N;\n"
    "\n"
    "      /* Optional \"fixed [w h]\": for the class that is defined by\n"
    "       * having no face in it, where a detector-driven crop can never\n"
    "       * fire.\n"
    "       */\n"
    "\n"
    "      if (argc > 4 && strcmp(argv[4], \"fixed\") == 0)\n"
    "        {\n"
    "          g_cap_fixed = 1;\n"
    "\n"
    "          if (argc > 6)\n"
    "            {\n"
    "              g_cap_fixed_w = atoi(argv[5]);\n"
    "              g_cap_fixed_h = atoi(argv[6]);\n"
    "            }\n"
    "        }\n"
    "    }\n",
    "argument parse")

# ------------------------------------------------------ 4. pin the crop

sub("          eye_cam_stage_add(EYE_CAM_ST_FACE, clock_systime_ticks() - t0);\n"
    "        }\n"
    "\n"
    "      t0 = clock_systime_ticks();\n",
    "          eye_cam_stage_add(EYE_CAM_ST_FACE, clock_systime_ticks() - t0);\n"
    "        }\n"
    "\n"
    "      /* Fixed-frame mode pins the crop here, after the detector has had\n"
    "       * its turn, so this is what the rest of the frame uses.  The\n"
    "       * detector still runs - it costs little next to the inference and\n"
    "       * its output is what the console reports - but nothing depends on\n"
    "       * it having found anything.\n"
    "       */\n"
    "\n"
    "      if (g_cap_fixed)\n"
    "        {\n"
    "          g_crop_w = g_cap_fixed_w;\n"
    "          g_crop_h = g_cap_fixed_h;\n"
    "          g_crop_x = (EYE_CAM_FRAME_W - g_crop_w) / 2;\n"
    "          g_crop_y = (EYE_CAM_FRAME_H - g_crop_h) / 2;\n"
    "          g_crop_ok = 1;\n"
    "        }\n"
    "\n"
    "      t0 = clock_systime_ticks();\n",
    "pin the crop")

# ------------------------------------------------------ 5. capture gate

sub("      if (g_cap_count > 0 && g_cap_saved < g_cap_count &&\n"
    "          face_ok && g_crop_ok)\n",
    "      if (g_cap_count > 0 && g_cap_saved < g_cap_count &&\n"
    "          (g_cap_fixed || (face_ok && g_crop_ok)))\n",
    "capture gate")

# ---------------------------------------------------- 6. the comment above

sub("       * Only frames the detector actually placed are written - a crop\n"
    "       * left over from the previous frame does not describe this one,\n"
    "       * and a training image whose box is wrong is worse than a\n"
    "       * missing one.\n"
    "       */\n",
    "       * Only frames the detector actually placed are written - a crop\n"
    "       * left over from the previous frame does not describe this one,\n"
    "       * and a training image whose box is wrong is worse than a\n"
    "       * missing one.\n"
    "       *\n"
    "       * Fixed-frame mode is the exception: there the crop describes\n"
    "       * the frame by construction, and the detector finding nothing is\n"
    "       * the normal case rather than a failure.\n"
    "       */\n",
    "comment")

# ------------------------------------------------------- 7. startup notice

sub("          eye_cam_out(\"eye_cam: capture \\\"%s\\\" %d frames to %s, \"\n"
    "                      \"from %04d\\n\",\n"
    "                      g_cap_label, g_cap_count, EYE_CAM_CAP_DIR,\n"
    "                      g_cap_seq);\n",
    "          eye_cam_out(\"eye_cam: capture \\\"%s\\\" %d frames to %s, \"\n"
    "                      \"from %04d%s\\n\",\n"
    "                      g_cap_label, g_cap_count, EYE_CAM_CAP_DIR,\n"
    "                      g_cap_seq,\n"
    "                      g_cap_fixed ? \" (fixed frame)\" : \"\");\n",
    "startup notice")

out = s
assert len(out) > before, "file shrank"
open(P, "w", encoding="utf-8").write(out)

back = open(P, encoding="utf-8").read()
assert back == out, "read-back mismatch"
for probe in ("EYE_CAM_CAP_FIXED_W", "g_cap_fixed_w", "g_cap_fixed || ",
              'strcmp(argv[4], "fixed")', "if (g_cap_fixed)\n"):
    assert probe in back, "missing: " + probe
    print("  ok  probe %s" % probe)

print("\n%d -> %d bytes, md5 %s"
      % (before, len(out), hashlib.md5(out.encode()).hexdigest()))
