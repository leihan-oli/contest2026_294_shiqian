# palm_cam — camera → ATON NPU palm detection (ST 995)

Captures one frame from the IMX335 (via DCMIPP into the LTDC framebuffer),
converts it to the model input tensor, runs ST's `033_palm_detection_full_quant`
network on the ATON NPU and prints the best palm detection: score, decoded box
and the 7 palm key points.

```
palm_cam [runs]        (default: 1 run; each run re-uses the captured frame)
```

## Data path

```
IMX335 -> DCMIPP -> LTDC framebuffer (800x480 RGB565, /dev/fb0)
                                   |
                                   v
              CPU: band-average downscale -> RGB888 192x192 @ 0x243a2000
                                   |
                                   v
              ATON NPU (palm995, /dev/aie0) -> 153216 B @ 0x24223700
                       head  [0, 145152)   2016 x 18 float32
                       score [145152, ...) 2016 x 1  float32
```

The application reads the *display* framebuffer instead of the camera buffer on
purpose: `/dev/fb0` already holds the frame the viewer sees, so the detected box
can be compared with what is on the panel, and `cam`/`cam_save` can dump the
same pixels.

## Input preprocessing

By default the whole 800x480 frame is **stretched** into the 192x192 input (both
axes scaled independently, i.e. the picture is squeezed horizontally by
800/480 = 1.67).  This is the layout that was measured to give the largest
detection margin on this board.

With `CONFIG_EXAMPLES_PALM_CAM_LETTERBOX` enabled the input keeps the camera
aspect ratio instead: the frame is scaled to 192x115 (both axes use the same
800/192 ratio) and the remaining 77 rows are left black.  That is the layout
ST's own application uses (`Projects/99_Applications/995_AI_Hand_Landmarks/Appli/
Core/Src/app_camera.c` selects a centred ROI with the display aspect ratio and
configures the DCMIPP neural pipe to `192 * 480/800 = 115` rows), but it costs
margin here - for one and the same frame the reference model scores:

| layout | reference logit | prob |
|---|---|---|
| stretch (default) | **+2.49** | 0.923 |
| centred square crop, 115x115 -> 192x192 | +1.95 | 0.875 |
| **letterbox, 192x115 + 77 black rows** | **+0.86** | 0.704 |
| centre crop, 154x115 -> 192x192 | +0.32 | 0.580 |

The ATON NPU and the PC reference disagree by up to ~3.5 logits on individual
inputs, so a 0.86 margin sits below that noise floor: with the letterbox layout a
palm that was clearly in view produced a peak logit of -2.59 on the board and was
reported as "no palm", while the same kind of frame scored +1.84 (detected) with
the stretch layout.

Both scales are also used to map the decoded box and key points back from the
192x192 input space to the 800x480 frame.

## Verdict line

Selector scores from this network are *logits*: a peak at or below 0 means the
sigmoid is at most 0.50, i.e. every candidate sits at the level the network
reserves for background.  To make that readable from a serial log the probe
prints one of

```
palm_cam: verdict palm (peak logit 1.84 > 0.00, prob 0.86, anchor 1764)
palm_cam: verdict no palm (peak logit 0.00 <= 0.00, so no anchor reaches prob 0.50: ...)
```

Note that the confidence scale on the NPU is not the reference one: the same
frame scores 0.86 here and 0.53 under the PC tflite interpreter, so decision
thresholds have to be calibrated on the board (see the ATON NPU validation
guide, section on the PC reference comparison).

## Dumps

With a writable mount point (`CONFIG_EXAMPLES_PALM_CAM_MOUNTPOINT`, default
`/mnt/sdcard`) the probe writes `palm_frame.ppm` (the exact 192x192 RGB picture
that was fed), `palm_in.bin` (the tensor) and `palm_out.bin` (the raw NPU pool
region) for PC-side inspection with `SoftwarePackage/palm_ref.py`,
`palm_analyze.py` and `palm_decode_cmp.py`.

## Configuration

| Option | Meaning |
|---|---|
| `EXAMPLES_PALM_CAM` | enable the application |
| `EXAMPLES_PALM_CAM_LETTERBOX` | keep the aspect ratio (default y) |
| `EXAMPLES_PALM_CAM_MOUNTPOINT` | where the dumps are written |
| `EXAMPLES_PALM_CAM_STACKSIZE` | task stack (default 8192) |

The NPU side is configured separately: `CONFIG_LIB_AI_ATON` with
`CONFIG_AI_ATON_MODEL_PALM995`.  `palm_cam` needs `CONFIG_STM32N6_VIDEO`
(camera) and `CONFIG_VIDEO_FB` (framebuffer).
