export class PovSim {
  constructor(module) {
    this._mod = module;
    this._simInit = module.cwrap('sim_init', 'boolean', ['number', 'number']);
    this._simResize = module.cwrap('sim_resize', 'boolean', ['number', 'number']);
    this._simGenerate = module.cwrap('sim_generate', null, ['number', 'number']);
    this._simGetFB = module.cwrap('sim_get_framebuffer', 'number', []);
    this._simNumSlices = module.cwrap('sim_num_slices', 'number', []);
    this._simNumLeds = module.cwrap('sim_num_leds', 'number', []);
    this._simSetBrightness = module.cwrap('sim_set_brightness', null, ['number']);
    this._simSetColor = module.cwrap('sim_set_color', null, ['number', 'number', 'number']);
    this._simSetPhaseOffset = module.cwrap('sim_set_phase_offset', null, ['number']);
    this._simSetText = module.cwrap('sim_set_text', null, ['string']);
    this._simNumPatterns = module.cwrap('sim_num_patterns', 'number', []);
    this._simPatternName = module.cwrap('sim_pattern_name', 'string', ['number']);
  }

  static async create(numSlices, numLeds) {
    const module = await PovSimModule();
    const sim = new PovSim(module);
    if (!sim._simInit(numSlices, numLeds)) {
      throw new Error('Failed to init framebuffer');
    }
    return sim;
  }

  generate(patternIndex, timeMs) {
    this._simGenerate(patternIndex, timeMs >>> 0);
  }

  resize(numSlices, numLeds) {
    return this._simResize(numSlices, numLeds);
  }

  get numSlices() { return this._simNumSlices(); }
  get numLeds() { return this._simNumLeds(); }

  get patternCount() { return this._simNumPatterns(); }
  patternName(i) { return this._simPatternName(i); }

  setBrightness(b) { this._simSetBrightness(b); }
  setColor(r, g, b) { this._simSetColor(r, g, b); }
  setPhaseOffset(o) { this._simSetPhaseOffset(o); }
  setText(t) { this._simSetText(t); }

  getPixel(slice, led) {
    const ptr = this._simGetFB();
    const numLeds = this.numLeds;
    const offset = (slice * numLeds + led) * 4;
    const heap = this._mod.HEAPU8;
    const br5 = heap[ptr + offset] & 0x1F;
    return {
      r: heap[ptr + offset + 3],
      g: heap[ptr + offset + 2],
      b: heap[ptr + offset + 1],
      brightness: br5,
    };
  }

  getFrontBufferPtr() {
    return this._simGetFB();
  }
}
