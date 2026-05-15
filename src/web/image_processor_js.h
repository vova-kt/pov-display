#pragma once
#include <pgmspace.h>

// Auto-generated from sim/js/image-processor.js — do not edit directly.
// Run tools/gen_embedded_js.sh to regenerate.

static const char IMAGE_PROCESSOR_JS[] PROGMEM = R"rawliteral(
// Shared image preprocessing for POV display (sim + MCU web UI).
// Loads via <script> tag — exposes globalThis.preprocessImage.

globalThis.preprocessImage = function(file, targetSize) {
  return new Promise((resolve, reject) => {
    const img = new Image();
    img.onload = () => {
      const side = Math.min(img.width, img.height);
      const sx = (img.width - side) / 2;
      const sy = (img.height - side) / 2;

      const cv = document.createElement('canvas');
      cv.width = targetSize;
      cv.height = targetSize;
      const ctx = cv.getContext('2d');
      ctx.imageSmoothingEnabled = true;
      ctx.imageSmoothingQuality = 'high';
      ctx.drawImage(img, sx, sy, side, side, 0, 0, targetSize, targetSize);

      const rgba = ctx.getImageData(0, 0, targetSize, targetSize).data;
      const rgb = new Uint8Array(targetSize * targetSize * 3);
      for (let i = 0, j = 0; i < rgba.length; i += 4, j += 3) {
        rgb[j]     = rgba[i];
        rgb[j + 1] = rgba[i + 1];
        rgb[j + 2] = rgba[i + 2];
      }

      URL.revokeObjectURL(img.src);
      resolve({ width: targetSize, height: targetSize, pixels: rgb });
    };
    img.onerror = () => {
      URL.revokeObjectURL(img.src);
      reject(new Error('Failed to load image'));
    };
    img.src = URL.createObjectURL(file);
  });
};
)rawliteral";
