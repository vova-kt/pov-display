export class PovSim {
  constructor(module) {
    this._mod = module;

    this._simInit          = module.cwrap('sim_init', 'boolean', ['number', 'number']);
    this._simResize        = module.cwrap('sim_resize', 'boolean', ['number', 'number']);
    this._simNumSlices     = module.cwrap('sim_num_slices', 'number', []);
    this._simNumLeds       = module.cwrap('sim_num_leds', 'number', []);
    this._simSetBrightness = module.cwrap('sim_set_brightness', null, ['number']);
    this._simSetColor      = module.cwrap('sim_set_color', null, ['number', 'number', 'number']);
    this._simSetPhaseOffset = module.cwrap('sim_set_phase_offset', null, ['number']);
    this._simSetText       = module.cwrap('sim_set_text', null, ['string']);
    this._simSetMirror     = module.cwrap('sim_set_mirror_pattern', null, ['number']);
    this._simSetRadialBalance = module.cwrap('sim_set_radial_balance', null, ['number']);
    this._simNumPatterns   = module.cwrap('sim_num_patterns', 'number', []);
    this._simPatternName   = module.cwrap('sim_pattern_name', 'string', ['number']);

    this._rendererInit          = module.cwrap('sim_renderer_init', 'boolean', []);
    this._rendererResize        = module.cwrap('sim_renderer_resize', null, ['number', 'number']);
    this._rendererSetHubFrac    = module.cwrap('sim_renderer_set_hub_frac', null, ['number']);
    this._rendererSetGapFrac    = module.cwrap('sim_renderer_set_gap_frac', null, ['number']);
    this._rendererSetShowOverruns    = module.cwrap('sim_renderer_set_show_overruns', null, ['number']);
    this._rendererSetShowHallMarker  = module.cwrap('sim_renderer_set_show_hall_marker', null, ['number']);
    this._rendererSetShowSliceGrid   = module.cwrap('sim_renderer_set_show_slice_grid', null, ['number']);
    this._rendererSetNumArms         = module.cwrap('sim_renderer_set_num_arms', null, ['number']);

    this._timingSetRpm        = module.cwrap('sim_timing_set_rpm', null, ['number']);
    this._timingSetRpmJitter  = module.cwrap('sim_timing_set_rpm_jitter', null, ['number']);
    this._timingSetHallJitter = module.cwrap('sim_timing_set_hall_jitter', null, ['number']);
    this._timingSetHallMiss   = module.cwrap('sim_timing_set_hall_miss', null, ['number']);
    this._timingSetTimerDrift = module.cwrap('sim_timing_set_timer_drift', null, ['number']);
    this._timingSetPatternLag = module.cwrap('sim_timing_set_pattern_lag', null, ['number']);
    this._timingSetSpiClock   = module.cwrap('sim_timing_set_spi_clock', null, ['number']);
    this._timingSetDisplayHz  = module.cwrap('sim_timing_set_display_hz', null, ['number']);

    this._simFrame = module.cwrap('sim_frame', null, ['number', 'number', 'number']);

    this._getActualRpm       = module.cwrap('sim_get_actual_rpm', 'number', []);
    this._getSliceIntervalUs = module.cwrap('sim_get_slice_interval_us', 'number', []);
    this._getSpiTransferUs   = module.cwrap('sim_get_spi_transfer_us', 'number', []);
    this._getHeadroomUs      = module.cwrap('sim_get_headroom_us', 'number', []);
    this._getFrameAge        = module.cwrap('sim_get_frame_age', 'number', []);
    this._getPatternGenMs    = module.cwrap('sim_get_pattern_gen_ms', 'number', []);
    this._getHallMissed      = module.cwrap('sim_get_hall_missed', 'boolean', []);
    this._getHasOverruns     = module.cwrap('sim_get_has_overruns', 'boolean', []);
  }

  static async create(numSlices, numLeds, canvas) {
    const module = await PovSimModule({ canvas });
    const sim = new PovSim(module);
    if (!sim._simInit(numSlices, numLeds)) {
      throw new Error('Failed to init framebuffer');
    }
    if (!sim._rendererInit()) {
      throw new Error('Failed to init WebGL2 renderer');
    }
    return sim;
  }

  resize(numSlices, numLeds) { return this._simResize(numSlices, numLeds); }
  get numSlices() { return this._simNumSlices(); }
  get numLeds() { return this._simNumLeds(); }
  get patternCount() { return this._simNumPatterns(); }
  patternName(i) { return this._simPatternName(i); }

  setBrightness(b)    { this._simSetBrightness(b); }
  setColor(r, g, b)   { this._simSetColor(r, g, b); }
  setPhaseOffset(o)   { this._simSetPhaseOffset(o); }
  setText(t)          { this._simSetText(t); }
  setMirror(m)        { this._simSetMirror(m ? 1 : 0); }
  setRadialBalance(v) { this._simSetRadialBalance(v ? 1 : 0); }

  rendererResize(w, h) { this._rendererResize(w, h); }
  setHubFraction(f)    { this._rendererSetHubFrac(f); }
  setGapFraction(f)    { this._rendererSetGapFrac(f); }
  setShowOverruns(v)   { this._rendererSetShowOverruns(v ? 1 : 0); }
  setShowHallMarker(v) { this._rendererSetShowHallMarker(v ? 1 : 0); }
  setShowSliceGrid(v)  { this._rendererSetShowSliceGrid(v ? 1 : 0); }
  setNumArms(n)        { this._rendererSetNumArms(n); }

  setRpm(v)          { this._timingSetRpm(v); }
  setRpmJitter(v)    { this._timingSetRpmJitter(v); }
  setHallJitter(v)   { this._timingSetHallJitter(v); }
  setHallMiss(v)     { this._timingSetHallMiss(v); }
  setTimerDrift(v)   { this._timingSetTimerDrift(v); }
  setPatternLag(v)   { this._timingSetPatternLag(v); }
  setSpiClock(v)     { this._timingSetSpiClock(v); }
  setDisplayHz(v)    { this._timingSetDisplayHz(v); }

  frame(dtMs, simTimeMs, patternIndex) {
    this._simFrame(dtMs, simTimeMs, patternIndex);
  }

  get actualRpm()       { return this._getActualRpm(); }
  get sliceIntervalUs() { return this._getSliceIntervalUs(); }
  get spiTransferUs()   { return this._getSpiTransferUs(); }
  get headroomUs()      { return this._getHeadroomUs(); }
  get frameAge()        { return this._getFrameAge(); }
  get patternGenMs()    { return this._getPatternGenMs(); }
  get hallMissed()      { return this._getHallMissed(); }
  get hasOverruns()     { return this._getHasOverruns(); }
}
