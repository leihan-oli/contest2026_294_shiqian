#!/usr/bin/env python3
"""Dataset capture for eye_cam.

Writes the crop the classifier is about to read to the card as an
uncompressed BMP, one file per frame, so the training set is built from
exactly the pixels the model sees at run time - same rectangle, same
geometry, same camera, same expansion tables.

Placement matters: the capture sits between the crop and the overlay.
The cyan box is drawn along the crop's own edge, so a capture taken after
the overlay would bake the annotation into the file.
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


# ------------------------------------------------------------ 1. include

sub("#include <sys/videoio.h>\n#include <sys/ioctl.h>\n",
    "#include <sys/videoio.h>\n#include <sys/ioctl.h>\n#include <sys/stat.h>\n",
    "include sys/stat.h")

# ---------------------------------------------------------- 2. constants

sub("#ifndef CONFIG_EXAMPLES_EYE_CAM_MOUNTPOINT\n"
    "#  define CONFIG_EXAMPLES_EYE_CAM_MOUNTPOINT \"/mnt/sdcard\"\n"
    "#endif\n",
    "#ifndef CONFIG_EXAMPLES_EYE_CAM_MOUNTPOINT\n"
    "#  define CONFIG_EXAMPLES_EYE_CAM_MOUNTPOINT \"/mnt/sdcard\"\n"
    "#endif\n"
    "\n"
    "/* Dataset capture.  The crop the classifier is fed is written out at\n"
    " * full size, one BMP per frame, so the training images come off the\n"
    " * same rectangle, the same camera and the same expansion tables as the\n"
    " * inference does.  Rescaling to the model's tensor is left to the PC:\n"
    " * doing it here would mean a second resampler whose behaviour has to\n"
    " * be kept in step with PIL's.\n"
    " */\n"
    "\n"
    "#define EYE_CAM_CAP_DIR   CONFIG_EXAMPLES_EYE_CAM_MOUNTPOINT \"/eye_ds\"\n"
    "#define EYE_CAM_CAP_META  EYE_CAM_CAP_DIR \"/meta.txt\"\n"
    "#define EYE_CAM_CAP_DEF_N 220\n"
    "#define EYE_CAM_CAP_MAX_W 512     /* sanity bounds on a crop */\n"
    "#define EYE_CAM_CAP_MAX_H 512\n",
    "constants")

# ------------------------------------------------------------ 3. globals

sub("static int  g_tap_w = -1;\n"
    "static int  g_tap_h = -1;\n",
    "static int  g_tap_w = -1;\n"
    "static int  g_tap_h = -1;\n"
    "\n"
    "/* Capture state.  Zero frames means the app runs as it always did, so\n"
    " * nothing on the inference path changes when the mode is off.\n"
    " */\n"
    "\n"
    "static FAR const char *g_cap_label;   /* class name in the file name */\n"
    "static int             g_cap_count;   /* frames asked for            */\n"
    "static int             g_cap_saved;   /* frames written              */\n"
    "static int             g_cap_seq = 1; /* first free file number      */\n"
    "static FAR FILE       *g_cap_meta;    /* meta.txt, append            */\n",
    "globals")

# ---------------------------------------------------------- 4. functions

FUNCS = r'''/****************************************************************************
 * Name: eye_cam_cap_put_u16 / eye_cam_cap_put_u32
 *
 * Description:
 *   Little endian stores for the BMP header.  BMP is defined that way, so
 *   these stay even on a big endian build.
 *
 ****************************************************************************/

static void eye_cam_cap_put_u16(FAR uint8_t *p, uint16_t v)
{
  p[0] = (uint8_t)(v & 0xff);
  p[1] = (uint8_t)((v >> 8) & 0xff);
}

static void eye_cam_cap_put_u32(FAR uint8_t *p, uint32_t v)
{
  p[0] = (uint8_t)(v & 0xff);
  p[1] = (uint8_t)((v >> 8) & 0xff);
  p[2] = (uint8_t)((v >> 16) & 0xff);
  p[3] = (uint8_t)((v >> 24) & 0xff);
}

/****************************************************************************
 * Name: eye_cam_cap_header
 *
 * Description:
 *   Write the 14 byte file header and 40 byte DIB header of a 24bpp
 *   uncompressed BMP.  No palette: the images are colour.
 *
 ****************************************************************************/

static void eye_cam_cap_header(FAR FILE *fp, int w, int h, uint32_t imgsz)
{
  FAR uint8_t hdr[54];

  memset(hdr, 0, sizeof(hdr));

  hdr[0] = 'B';
  hdr[1] = 'M';
  eye_cam_cap_put_u32(&hdr[2], 54 + imgsz);       /* file size          */
  eye_cam_cap_put_u32(&hdr[10], 54);              /* pixel data offset  */
  eye_cam_cap_put_u32(&hdr[14], 40);              /* DIB header size    */
  eye_cam_cap_put_u32(&hdr[18], (uint32_t)w);
  eye_cam_cap_put_u32(&hdr[22], (uint32_t)h);
  eye_cam_cap_put_u16(&hdr[26], 1);               /* colour planes      */
  eye_cam_cap_put_u16(&hdr[28], 24);              /* bits per pixel     */
  eye_cam_cap_put_u32(&hdr[34], imgsz);
  eye_cam_cap_put_u32(&hdr[38], 2835);            /* 72 dpi             */
  eye_cam_cap_put_u32(&hdr[42], 2835);
  eye_cam_cap_put_u32(&hdr[46], 0);               /* no palette         */
  eye_cam_cap_put_u32(&hdr[50], 0);

  fwrite(hdr, 1, sizeof(hdr), fp);
}

/****************************************************************************
 * Name: eye_cam_cap_row
 *
 * Description:
 *   Convert one crop row out of the framebuffer, one output pixel per
 *   source pixel.  The crop follows the face, so near the edge of the
 *   frame it hangs over it: outside pixels are black, matching what the
 *   classifier's input gets from the same rectangle.
 *
 *   Channel expansion goes through g_exp5/g_exp6, the tables the input
 *   path uses, so a pixel means the same thing in a captured file and in
 *   a tensor.
 *
 ****************************************************************************/

static void eye_cam_cap_row(FAR const uint16_t *fb, uint32_t stride_px,
                            int rx, int ry, int rw, int y,
                            FAR uint8_t *row)
{
  int sy = ry + y;
  int x;

  if (sy < 0 || sy >= EYE_CAM_FRAME_H)
    {
      memset(row, 0, (size_t)rw * 3);
      return;
    }

  {
    FAR const uint16_t *srow = &fb[(size_t)sy * stride_px];

    for (x = 0; x < rw; x++)
      {
        int sx = rx + x;
        uint16_t px = 0;

        if (sx >= 0 && sx < EYE_CAM_FRAME_W)
          {
            px = srow[sx];
          }

        /* BMP pixel order is B, G, R */

        row[x * 3 + 0] = g_exp5[px & 0x1f];
        row[x * 3 + 1] = g_exp6[(px >> 5) & 0x3f];
        row[x * 3 + 2] = g_exp5[(px >> 11) & 0x1f];
      }
  }
}

/****************************************************************************
 * Name: eye_cam_cap_next_seq
 *
 * Description:
 *   First unused file number for a label.  Probing with fopen rather than
 *   scanning the directory keeps the FATFS work to one entry per frame
 *   already present, and means a second run of the same class adds to the
 *   set instead of overwriting it - a bad batch can be deleted and redone
 *   without touching the good frames.
 *
 ****************************************************************************/

static int eye_cam_cap_next_seq(FAR const char *label)
{
  char path[160];
  FAR FILE *fp;
  int n;

  for (n = 1; n < 10000; n++)
    {
      snprintf(path, sizeof(path), EYE_CAM_CAP_DIR "/%s_%04d.bmp",
               label, n);

      fp = fopen(path, "rb");
      if (fp == NULL)
        {
          return n;
        }

      fclose(fp);
    }

  return -1;
}

/****************************************************************************
 * Name: eye_cam_cap_save
 *
 * Description:
 *   Write one frame's crop.  The image is streamed a row at a time: a
 *   whole 288x139 colour image in RAM would be 120 KB of contiguous heap
 *   for nothing, since the row is all that changes between iterations.
 *
 *   Rows go out bottom up, which is how BMP stores them, and the pad bytes
 *   that bring a row up to a multiple of four are zeroed once rather than
 *   per row - they are the same bytes every time.
 *
 ****************************************************************************/

static int eye_cam_cap_save(FAR const char *label, int seq, uint32_t stride_px,
                            FAR const struct eye_cam_face_s *face,
                            FAR FILE *meta)
{
  char path[160];
  FAR uint8_t *row;
  FAR FILE *fp;
  int rw = g_crop_w;
  int rh = g_crop_h;
  int rowsz;
  int y;

  if (rw <= 0 || rh <= 0 ||
      rw > EYE_CAM_CAP_MAX_W || rh > EYE_CAM_CAP_MAX_H)
    {
      return -EINVAL;
    }

  rowsz = (rw * 3 + 3) & ~3;

  row = malloc((size_t)rowsz);
  if (row == NULL)
    {
      return -ENOMEM;
    }

  memset(row, 0, (size_t)rowsz);

  snprintf(path, sizeof(path), EYE_CAM_CAP_DIR "/%s_%04d.bmp", label, seq);

  fp = fopen(path, "wb");
  if (fp == NULL)
    {
      free(row);
      return -errno;
    }

  eye_cam_cap_header(fp, rw, rh, (uint32_t)rowsz * (uint32_t)rh);

  for (y = rh - 1; y >= 0; y--)
    {
      eye_cam_cap_row(g_fb, stride_px, g_crop_x, g_crop_y, rw, y, row);

      if (fwrite(row, 1, (size_t)rowsz, fp) != (size_t)rowsz)
        {
          break;
        }
    }

  fclose(fp);
  free(row);

  /* One line per frame, so a badly placed crop can be found and dropped
   * on the PC side without opening a single image.  Flushed each time:
   * the card is pulled at the end of a session and an unflushed tail
   * would lose exactly the frames that show where the box drifted.
   */

  if (meta != NULL)
    {
      fprintf(meta, "%s %04d %d %d %d %d %.3f %.1f %.1f %.1f\n",
              label, seq, g_crop_x, g_crop_y, rw, rh,
              (double)face->score, (double)face->ex, (double)face->ey,
              (double)face->dx);
      fflush(meta);
    }

  return 0;
}

'''

sub("int main(int argc, FAR char *argv[])\n",
    FUNCS + "int main(int argc, FAR char *argv[])\n",
    "capture functions")

# ------------------------------------------------------- 5. argument parse

sub("  if (argc > 2)\n"
    "    {\n"
    "      hold_ms = atoi(argv[2]);\n"
    "    }\n",
    "  if (argc > 2)\n"
    "    {\n"
    "      hold_ms = atoi(argv[2]);\n"
    "    }\n"
    "\n"
    "  /* eye_cam capture <label> [count]\n"
    "   *\n"
    "   * One class per run, and the run stops when the batch is full.\n"
    "   * The next label is a separate invocation so a batch can be checked\n"
    "   * - or redone - before the next one starts.\n"
    "   */\n"
    "\n"
    "  if (argc > 1 && strcmp(argv[1], \"capture\") == 0)\n"
    "    {\n"
    "      g_cap_label = (argc > 2) ? argv[2] : \"sample\";\n"
    "      g_cap_count = (argc > 3) ? atoi(argv[3]) : EYE_CAM_CAP_DEF_N;\n"
    "    }\n",
    "argument parse")

# ------------------------------------------------------- 6. capture set-up

sub("  for (i = 0; i < iters || iters <= 0; i++)\n",
    "  /* Open the capture directory before the first frame, so a missing\n"
    "   * card is reported once at the start rather than as a file error on\n"
    "   * every frame.\n"
    "   */\n"
    "\n"
    "  if (g_cap_count > 0)\n"
    "    {\n"
    "      if (mkdir(EYE_CAM_CAP_DIR, 0777) < 0 && errno != EEXIST)\n"
    "        {\n"
    "          eye_cam_out(\"eye_cam: capture off: mkdir %s failed: %d\\n\",\n"
    "                      EYE_CAM_CAP_DIR, errno);\n"
    "          g_cap_count = 0;\n"
    "        }\n"
    "      else if ((g_cap_seq = eye_cam_cap_next_seq(g_cap_label)) < 0)\n"
    "        {\n"
    "          eye_cam_out(\"eye_cam: capture off: no free file number\\n\");\n"
    "          g_cap_count = 0;\n"
    "        }\n"
    "      else\n"
    "        {\n"
    "          g_cap_meta = fopen(EYE_CAM_CAP_META, \"a\");\n"
    "          if (g_cap_meta != NULL && ftell(g_cap_meta) == 0)\n"
    "            {\n"
    "              fprintf(g_cap_meta,\n"
    "                      \"# label seq crop_x crop_y crop_w crop_h \"\n"
    "                      \"score eye_x eye_y dx\\n\");\n"
    "              fflush(g_cap_meta);\n"
    "            }\n"
    "\n"
    "          eye_cam_out(\"eye_cam: capture \\\"%s\\\" %d frames to %s, \"\n"
    "                      \"from %04d\\n\",\n"
    "                      g_cap_label, g_cap_count, EYE_CAM_CAP_DIR,\n"
    "                      g_cap_seq);\n"
    "        }\n"
    "    }\n"
    "\n"
    "  for (i = 0; i < iters || iters <= 0; i++)\n",
    "capture set-up")

# -------------------------------------------------- 7. the capture itself

sub("      eye_cam_taps_for(g_crop_w, g_crop_h);\n"
    "      eye_cam_roi_to_input(g_fb, pinfo.stride, input, g_crop_x, g_crop_y);\n"
    "      up_clean_dcache((uintptr_t)input, (uintptr_t)input + EYE_CAM_IN_LEN);\n"
    "\n"
    "      if ((i % 10) == 0)\n",
    "      eye_cam_taps_for(g_crop_w, g_crop_h);\n"
    "      eye_cam_roi_to_input(g_fb, pinfo.stride, input, g_crop_x, g_crop_y);\n"
    "      up_clean_dcache((uintptr_t)input, (uintptr_t)input + EYE_CAM_IN_LEN);\n"
    "\n"
    "      /* Capture the crop that was just built, here rather than after\n"
    "       * the overlay: the box is drawn along the crop's own edge, and a\n"
    "       * file written afterwards would carry the annotation.\n"
    "       *\n"
    "       * Only frames the detector actually placed are written - a crop\n"
    "       * left over from the previous frame does not describe this one,\n"
    "       * and a training image whose box is wrong is worse than a\n"
    "       * missing one.\n"
    "       */\n"
    "\n"
    "      if (g_cap_count > 0 && g_cap_saved < g_cap_count &&\n"
    "          face_ok && g_crop_ok)\n"
    "        {\n"
    "          if (eye_cam_cap_save(g_cap_label, g_cap_seq + g_cap_saved,\n"
    "                               g_stride_px, &face, g_cap_meta) == 0)\n"
    "            {\n"
    "              g_cap_saved++;\n"
    "\n"
    "              if ((g_cap_saved % 10) == 0 || g_cap_saved == g_cap_count)\n"
    "                {\n"
    "                  eye_cam_out(\"eye_cam: capture %s %d/%d (%dx%d)\\n\",\n"
    "                              g_cap_label, g_cap_saved, g_cap_count,\n"
    "                              g_crop_w, g_crop_h);\n"
    "                }\n"
    "            }\n"
    "        }\n"
    "\n"
    "      if ((i % 10) == 0)\n",
    "capture call")

# ------------------------------------------------------- 8. batch finished

sub("      eye_cam_stage_add(EYE_CAM_ST_TOTAL, clock_systime_ticks() - tframe);\n"
    "\n"
    "      if ((i % 15) == 0)\n"
    "        {\n"
    "          eye_cam_stage_report(i + 1);\n"
    "        }\n"
    "    }\n",
    "      eye_cam_stage_add(EYE_CAM_ST_TOTAL, clock_systime_ticks() - tframe);\n"
    "\n"
    "      if ((i % 15) == 0)\n"
    "        {\n"
    "          eye_cam_stage_report(i + 1);\n"
    "        }\n"
    "\n"
    "      if (g_cap_count > 0 && g_cap_saved >= g_cap_count)\n"
    "        {\n"
    "          break;\n"
    "        }\n"
    "    }\n",
    "batch finished")

# ------------------------------------------------------------- 9. cleanup

sub("  eye_cam_stage_report(i);\n"
    "\n"
    "  eye_cam_out(\"eye_cam: done\\n\");\n",
    "  eye_cam_stage_report(i);\n"
    "\n"
    "  if (g_cap_meta != NULL)\n"
    "    {\n"
    "      fclose(g_cap_meta);\n"
    "      g_cap_meta = NULL;\n"
    "    }\n"
    "\n"
    "  if (g_cap_count > 0)\n"
    "    {\n"
    "      eye_cam_out(\"eye_cam: capture %s: %d/%d frames, files %04d..%04d\\n\",\n"
    "                  g_cap_label, g_cap_saved, g_cap_count, g_cap_seq,\n"
    "                  g_cap_seq + (g_cap_saved > 0 ? g_cap_saved - 1 : 0));\n"
    "    }\n"
    "\n"
    "  eye_cam_out(\"eye_cam: done\\n\");\n",
    "cleanup")

out = s
assert len(out) > before, "file shrank"
open(P, "w", encoding="utf-8").write(out)

back = open(P, encoding="utf-8").read()
assert back == out, "read-back mismatch"

probes = ("EYE_CAM_CAP_DIR", "EYE_CAM_CAP_META", "eye_cam_cap_save",
          "eye_cam_cap_row", "eye_cam_cap_next_seq", "g_cap_meta",
          "strcmp(argv[1], \"capture\")", "include <sys/stat.h>")
for p in probes:
    c = back.count(p)
    assert c > 0, "probe missing: " + p
    print("  ok  probe %-32s x%d" % (p, c))

print("\n%d -> %d bytes, md5 %s"
      % (before, len(out), hashlib.md5(out.encode()).hexdigest()))
