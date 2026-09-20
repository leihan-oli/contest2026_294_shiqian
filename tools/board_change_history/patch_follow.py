#!/usr/bin/env python3
"""Move the verdict frame and its label onto the crop rectangle.

They were pinned to the fixed ROI, which stopped meaning anything the moment
the crop started following the face: a coloured box parked in the middle of
the screen, next to a face it has nothing to do with, reads as a fault.

The cyan box is not replaced - it and the verdict frame are offset by one
stroke so both stay legible: the colour carries the verdict, the cyan says
which pixels went into the classifier.
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

sub(
    "#define EYE_CAM_LABEL_SCALE     4\n",
    "/* The verdict frame and its label follow the crop rectangle now that the\n"
    " * crop follows the face.  The label sits this far above the frame when\n"
    " * there is room, and the same distance below it when there is not.\n"
    " */\n"
    "\n"
    "#define EYE_CAM_LABEL_GAP       30\n"
    "#define EYE_CAM_VERDICT_PAD     3     /* frame drawn outside the crop */\n"
    "#define EYE_CAM_DRAW_MARGIN     80    /* cache push covers this much */\n"
    "\n"
    "#define EYE_CAM_LABEL_SCALE     4\n",
    "constants",
)

# ------------------------------------------------- 2. frame and label follow

sub(
    "        /* Frame the ROI in the colour of the verdict, then print the verdict\n"
    "         * above it - outside the region the model looks at, so the annotation\n"
    "         * never becomes part of the input of the next iteration.\n"
    "         */\n"
    "\n"
    "        eye_cam_rect(EYE_CAM_ROI_X, EYE_CAM_ROI_Y, EYE_CAM_ROI_W,\n"
    "                     EYE_CAM_ROI_H, 3, g_eye_cam_color[best]);\n"
    "\n"
    "        {\n"
    "          char txt[40];\n"
    "\n"
    "          snprintf(txt, sizeof(txt), \"%s %d%%\", g_eye_cam_name[best],\n"
    "                   (int)(100.0f * p[best] / sum + 0.5f));\n"
    "\n"
    "          eye_cam_label(EYE_CAM_ROI_X, EYE_CAM_LABEL_Y, txt,\n"
    "                        EYE_CAM_LABEL_SCALE, g_eye_cam_color[best]);\n"
    "        }\n",
    "        /* Frame the window the classifier actually saw, in the colour of the\n"
    "         * verdict, and print the verdict with it.\n"
    "         *\n"
    "         * Drawn one stroke wider than the cyan box rather than on top of\n"
    "         * it, so the two stay distinguishable: the colour carries the\n"
    "         * verdict, the cyan says which pixels were cropped.\n"
    "         *\n"
    "         * Still outside the cropped region, so the annotation can never\n"
    "         * become part of the next iteration's input.\n"
    "         */\n"
    "\n"
    "        eye_cam_rect(g_crop_x - EYE_CAM_VERDICT_PAD,\n"
    "                     g_crop_y - EYE_CAM_VERDICT_PAD,\n"
    "                     g_crop_w + 2 * EYE_CAM_VERDICT_PAD,\n"
    "                     g_crop_h + 2 * EYE_CAM_VERDICT_PAD,\n"
    "                     3, g_eye_cam_color[best]);\n"
    "\n"
    "        {\n"
    "          char txt[40];\n"
    "          int  ly = g_crop_y - EYE_CAM_LABEL_GAP;\n"
    "\n"
    "          snprintf(txt, sizeof(txt), \"%s %d%%\", g_eye_cam_name[best],\n"
    "                   (int)(100.0f * p[best] / sum + 0.5f));\n"
    "\n"
    "          /* Above the frame when there is room, below it when the face\n"
    "           * sits high enough that there is not - a label off the top of\n"
    "           * the screen is worth nothing.\n"
    "           */\n"
    "\n"
    "          if (ly < EYE_CAM_LABEL_GAP)\n"
    "            {\n"
    "              ly = g_crop_y + g_crop_h + EYE_CAM_LABEL_GAP;\n"
    "            }\n"
    "\n"
    "          eye_cam_label(g_crop_x, ly, txt,\n"
    "                        EYE_CAM_LABEL_SCALE, g_eye_cam_color[best]);\n"
    "        }\n",
    "frame and label follow the crop",
)

# ----------------------------------------------------- 3. widen the push

sub(
    "            y0 = cy - 4;\n"
    "            y1 = cy + ch + 4;\n",
    "            /* Everything drawn sits near the crop: the verdict frame\n"
    "             * just outside it, the label a line beyond that, the eye\n"
    "             * markers inside.  Deriving the push from the crop covers\n"
    "             * all three wherever the face happens to be.\n"
    "             */\n"
    "\n"
    "            y0 = cy - EYE_CAM_DRAW_MARGIN;\n"
    "            y1 = cy + ch + EYE_CAM_DRAW_MARGIN;\n",
    "widen the cache push",
)

out = s
assert len(out) > before, "file shrank"
open(P, "w", encoding="utf-8").write(out)

back = open(P, encoding="utf-8").read()
assert back == out, "read-back mismatch"
for probe in ("EYE_CAM_VERDICT_PAD", "EYE_CAM_DRAW_MARGIN",
              "g_crop_x - EYE_CAM_VERDICT_PAD", "ly = g_crop_y + g_crop_h"):
    c = back.count(probe)
    assert c > 0, "probe missing: " + probe
    print("  ok  probe %-34s x%d" % (probe, c))

print("\n%d -> %d bytes, md5 %s"
      % (before, len(out), hashlib.md5(out.encode()).hexdigest()))
