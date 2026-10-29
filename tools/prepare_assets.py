#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
# Copyright (C) 2026 GuitarFros
"""
Regenerates the GUI assets used by the plugin from the original artwork:

  python3 tools/prepare_assets.py <pedal_blank.jpg> <knob.jpg> [out_dir]

  pedal_blank.jpg : 835x1210 pedal body (LEDs lit, knob positions empty)
  knob.jpg        : top view of the knob on a white background

Outputs (in Assets/):
  pedal_base.jpg  : pedal with LEDs / toggle lever removed (all drawn live by the plugin)
  knob.png        : knob cut out with a circular alpha mask (480x480)

Needs: pillow, numpy, opencv-python
"""
import sys
import cv2
import numpy as np
from PIL import Image

LED_X   = [118.5, 194, 271, 348.5, 424.5, 500.5, 577.5, 655, 731.5]
LED_Y   = 602
LED_HUE = [0, 18, 28, 42, 60, 42, 28, 18, 0]      # OpenCV hue (0..180) of the *unlit* lens
ACT_LED = (170.7, 923.3)
TOGGLE  = (668, 785)                              # lever ball in the source image


def feather_circle(h, w, cx, cy, r, f=3.0):
    yy, xx = np.mgrid[0:h, 0:w]
    d = np.sqrt((xx - cx) ** 2 + (yy - cy) ** 2)
    return np.clip((r - d) / f, 0, 1)[..., None]


def main():
    pedal_path, knob_path = sys.argv[1], sys.argv[2]
    out = sys.argv[3] if len(sys.argv) > 3 else "Assets"

    img = cv2.imread(pedal_path)
    h, w = img.shape[:2]
    ref = img.copy()

    # --- LED strip: use the already-dark outer LED as the "unlit" template -----------
    R = 36
    tx, ty = int(round(LED_X[0])), LED_Y
    tmpl = ref[ty - R:ty + R, tx - R:tx + R].copy()
    lens = feather_circle(2 * R, 2 * R, R, R, 25, 4)
    for i, cx in enumerate(LED_X):
        patch = tmpl.copy()
        if LED_HUE[i] != 0:
            hsv = cv2.cvtColor(patch, cv2.COLOR_BGR2HSV).astype(np.int32)
            hsv[..., 0] = (hsv[..., 0] + LED_HUE[i]) % 180
            tinted = cv2.cvtColor(hsv.astype(np.uint8), cv2.COLOR_HSV2BGR)
            patch = (tinted * lens + patch * (1 - lens)).astype(np.uint8)
        x0, y0 = int(round(cx)) - R, LED_Y - R
        m = feather_circle(2 * R, 2 * R, R, R, R - 2, 6)
        roi = img[y0:y0 + 2 * R, x0:x0 + 2 * R]
        img[y0:y0 + 2 * R, x0:x0 + 2 * R] = (patch * m + roi * (1 - m)).astype(np.uint8)

    # --- ACTIVE led lens: copy the dark lens from the template -----------------------
    ax, ay = int(round(ACT_LED[0])), int(round(ACT_LED[1]))
    r2 = 30
    lens_t = cv2.resize(tmpl[R - 25:R + 25, R - 25:R + 25], (2 * r2, 2 * r2), interpolation=cv2.INTER_CUBIC)
    m = feather_circle(2 * r2, 2 * r2, r2, r2, r2 - 2, 4)
    roi = img[ay - r2:ay + r2, ax - r2:ax + r2]
    img[ay - r2:ay + r2, ax - r2:ax + r2] = (lens_t * m + roi * (1 - m)).astype(np.uint8)

    # --- toggle: erase the lever ball (it is drawn live) -----------------------------
    mask = np.zeros((h, w), np.uint8)
    cv2.circle(mask, TOGGLE, 27, 255, -1)
    img = cv2.inpaint(img, mask, 9, cv2.INPAINT_TELEA)

    cv2.imwrite(f"{out}/pedal_base.jpg", img, [cv2.IMWRITE_JPEG_QUALITY, 94])

    # --- knob cut-out -----------------------------------------------------------------
    k = cv2.imread(knob_path)
    # Knob disc measured on the 1600x1600 source (automatic thresholding also grabs the
    # drop shadow, so a fixed circle is more reliable).
    cx, cy, r = 800.0, 797.0, 341.0
    pad = int(r) + 4
    crop = k[int(cy) - pad:int(cy) + pad, int(cx) - pad:int(cx) + pad]
    hh, ww = crop.shape[:2]
    alpha = feather_circle(hh, ww, ww / 2, hh / 2, r, 3)[..., 0]
    rgba = np.dstack([crop, (alpha * 255).astype(np.uint8)])
    rgba = cv2.resize(rgba, (480, 480), interpolation=cv2.INTER_AREA)
    Image.fromarray(cv2.cvtColor(rgba, cv2.COLOR_BGRA2RGBA)).save(f"{out}/knob.png", optimize=True)
    print("knob circle:", cx, cy, r)


if __name__ == "__main__":
    main()
