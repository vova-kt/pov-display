export class RadialRenderer {
  constructor(canvas) {
    this._canvas = canvas;
    this._ctx = canvas.getContext('2d');
    this._lookupTable = null;
    this._lookupDims = { w: 0, h: 0, numSlices: 0, numLeds: 0 };
    this._imageData = null;
    this._showOverruns = true;
    this._showSliceGrid = false;
    this._showHallMarker = true;
  }

  set showOverruns(v) { this._showOverruns = v; }
  set showSliceGrid(v) { this._showSliceGrid = v; }
  set showHallMarker(v) { this._showHallMarker = v; }

  _buildLookupTable(numSlices, numLeds) {
    const w = this._canvas.width;
    const h = this._canvas.height;

    if (this._lookupDims.w === w &&
        this._lookupDims.h === h &&
        this._lookupDims.numSlices === numSlices &&
        this._lookupDims.numLeds === numLeds) {
      return;
    }

    const cx = w / 2;
    const cy = h / 2;
    const outerR = Math.min(cx, cy) - 2;
    const innerR = outerR * 0.15;
    const ledHeight = (outerR - innerR) / numLeds;

    const table = new Int16Array(w * h * 2);
    table.fill(-1);

    for (let y = 0; y < h; y++) {
      for (let x = 0; x < w; x++) {
        const dx = x - cx;
        const dy = y - cy;
        const dist = Math.sqrt(dx * dx + dy * dy);

        if (dist < innerR || dist > outerR) continue;

        let angle = Math.atan2(dy, dx);
        if (angle < 0) angle += Math.PI * 2;

        const sliceIndex = Math.floor((angle / (Math.PI * 2)) * numSlices) % numSlices;
        const ledIndex = Math.floor((dist - innerR) / ledHeight);
        const clampedLed = Math.min(ledIndex, numLeds - 1);

        const idx = (y * w + x) * 2;
        table[idx] = sliceIndex;
        table[idx + 1] = clampedLed;
      }
    }

    this._lookupTable = table;
    this._lookupDims = { w, h, numSlices, numLeds };
    this._imageData = this._ctx.createImageData(w, h);
  }

  render(sim, revolution, phaseOffset) {
    const numSlices = sim.numSlices;
    const numLeds = sim.numLeds;
    const w = this._canvas.width;
    const h = this._canvas.height;

    this._buildLookupTable(numSlices, numLeds);

    const fbPtr = sim.getFrontBufferPtr();
    const heap = sim._mod.HEAPU8;
    const pixels = this._imageData.data;

    const phaseSlices = Math.round((phaseOffset / 360) * numSlices);

    const angleRemap = revolution ? this._buildAngleRemap(revolution, numSlices) : null;

    for (let y = 0; y < h; y++) {
      for (let x = 0; x < w; x++) {
        const ltIdx = (y * w + x) * 2;
        const pxIdx = (y * w + x) * 4;

        let nominalSlice = this._lookupTable[ltIdx];
        if (nominalSlice < 0) {
          pixels[pxIdx] = 10;
          pixels[pxIdx + 1] = 10;
          pixels[pxIdx + 2] = 10;
          pixels[pxIdx + 3] = 255;
          continue;
        }

        let fbSlice;
        if (angleRemap) {
          fbSlice = angleRemap[nominalSlice];
        } else {
          fbSlice = nominalSlice;
        }

        fbSlice = ((fbSlice + phaseSlices) % numSlices + numSlices) % numSlices;
        const ledIndex = this._lookupTable[ltIdx + 1];

        const offset = (fbSlice * numLeds + ledIndex) * 4;
        const br5 = heap[fbPtr + offset] & 0x1F;
        const b = heap[fbPtr + offset + 1];
        const g = heap[fbPtr + offset + 2];
        const r = heap[fbPtr + offset + 3];

        const scale = br5 / 31;
        pixels[pxIdx]     = (r * scale) | 0;
        pixels[pxIdx + 1] = (g * scale) | 0;
        pixels[pxIdx + 2] = (b * scale) | 0;
        pixels[pxIdx + 3] = 255;
      }
    }

    this._ctx.putImageData(this._imageData, 0, 0);

    if (revolution) {
      if (this._showOverruns) this._drawOverruns(revolution, numSlices);
      if (this._showHallMarker) this._drawHallMarker(revolution);
      if (this._showSliceGrid) this._drawSliceGrid(numSlices);
    }
  }

  _buildAngleRemap(revolution, numSlices) {
    if (!revolution.slices || revolution.slices.length === 0) return null;

    const hasDistortion = revolution.slices.some(
      s => Math.abs(s.actualAngle - s.nominalAngle) > 1e-6
    );
    if (!hasDistortion) return null;

    const remap = new Int16Array(numSlices);

    for (let screenSlice = 0; screenSlice < numSlices; screenSlice++) {
      const screenAngle = (screenSlice / numSlices) * Math.PI * 2;

      let closest = 0;
      let closestDist = Infinity;
      for (const s of revolution.slices) {
        let diff = Math.abs(s.actualAngle - screenAngle);
        if (diff > Math.PI) diff = Math.PI * 2 - diff;
        if (diff < closestDist) {
          closestDist = diff;
          closest = s.sliceIndex;
        }
      }
      remap[screenSlice] = closest;
    }
    return remap;
  }

  _drawOverruns(revolution, numSlices) {
    const ctx = this._ctx;
    const cx = this._canvas.width / 2;
    const cy = this._canvas.height / 2;
    const outerR = Math.min(cx, cy) - 2;

    for (const s of revolution.slices) {
      if (!s.overrun) continue;
      const angle = s.nominalAngle;
      const nextAngle = angle + (Math.PI * 2) / numSlices;

      ctx.beginPath();
      ctx.moveTo(cx, cy);
      ctx.arc(cx, cy, outerR, angle, nextAngle);
      ctx.closePath();
      ctx.fillStyle = 'rgba(255, 0, 0, 0.25)';
      ctx.fill();
    }
  }

  _drawHallMarker(revolution) {
    const ctx = this._ctx;
    const cx = this._canvas.width / 2;
    const cy = this._canvas.height / 2;
    const outerR = Math.min(cx, cy) - 2;

    const angle = revolution.hallOffsetUs
      ? (revolution.hallOffsetUs / (revolution.periodMs * 1000)) * Math.PI * 2
      : 0;

    ctx.beginPath();
    ctx.moveTo(cx, cy);
    ctx.lineTo(
      cx + Math.cos(angle) * outerR,
      cy + Math.sin(angle) * outerR
    );
    ctx.strokeStyle = 'rgba(0, 255, 0, 0.6)';
    ctx.lineWidth = 2;
    ctx.stroke();
  }

  _drawSliceGrid(numSlices) {
    const ctx = this._ctx;
    const cx = this._canvas.width / 2;
    const cy = this._canvas.height / 2;
    const outerR = Math.min(cx, cy) - 2;
    const innerR = outerR * 0.15;

    const step = Math.max(1, Math.floor(numSlices / 36));

    ctx.strokeStyle = 'rgba(255, 255, 255, 0.15)';
    ctx.lineWidth = 0.5;
    for (let i = 0; i < numSlices; i += step) {
      const angle = (i / numSlices) * Math.PI * 2;
      ctx.beginPath();
      ctx.moveTo(
        cx + Math.cos(angle) * innerR,
        cy + Math.sin(angle) * innerR
      );
      ctx.lineTo(
        cx + Math.cos(angle) * outerR,
        cy + Math.sin(angle) * outerR
      );
      ctx.stroke();
    }
  }

  invalidateLookup() {
    this._lookupDims = { w: 0, h: 0, numSlices: 0, numLeds: 0 };
  }
}
