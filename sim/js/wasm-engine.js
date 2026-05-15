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
    this._simSetTextMode   = module.cwrap('sim_set_text_mode', null, ['number']);
    this._simSetTextDelay  = module.cwrap('sim_set_text_delay', null, ['number']);
    this._simSetMirror     = module.cwrap('sim_set_mirror_pattern', null, ['number']);
    this._simSetRadialBalance = module.cwrap('sim_set_radial_balance', null, ['number']);
    this._simLoadImage     = module.cwrap('sim_load_image', 'boolean', ['number', 'number', 'number']);
    this._simNumPatterns   = module.cwrap('sim_num_patterns', 'number', []);
    this._simPatternName   = module.cwrap('sim_pattern_name', 'string', ['number']);

    // Animation introspection
    this._simNumAnimations       = module.cwrap('sim_num_animations', 'number', []);
    this._simAnimationName       = module.cwrap('sim_animation_name', 'string', ['number']);
    this._simAnimationKey        = module.cwrap('sim_animation_key', 'string', ['number']);
    this._simAnimParamCount      = module.cwrap('sim_animation_param_count', 'number', ['number']);
    this._simAnimParamKey        = module.cwrap('sim_animation_param_key', 'string', ['number', 'number']);
    this._simAnimParamLabel      = module.cwrap('sim_animation_param_label', 'string', ['number', 'number']);
    this._simAnimParamValue      = module.cwrap('sim_animation_param_value', 'number', ['number', 'number']);
    this._simAnimParamDefault    = module.cwrap('sim_animation_param_default', 'number', ['number', 'number']);
    this._simAnimParamMin        = module.cwrap('sim_animation_param_min', 'number', ['number', 'number']);
    this._simAnimParamMax        = module.cwrap('sim_animation_param_max', 'number', ['number', 'number']);
    this._simAnimParamPresetCount = module.cwrap('sim_animation_param_preset_count', 'number', ['number', 'number']);
    this._simAnimParamPresetLabel = module.cwrap('sim_animation_param_preset_label', 'string', ['number', 'number', 'number']);
    this._simAnimParamPresetValue = module.cwrap('sim_animation_param_preset_value', 'number', ['number', 'number', 'number']);
    this._simSetAnimParam        = module.cwrap('sim_set_animation_param', null, ['number', 'number', 'number']);

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
    // Register JSON exports
    sim._getSettingsJson = module.cwrap('sim_get_settings_json', 'string', []);
    sim._applySettingsJson = module.cwrap('sim_apply_settings_json', 'boolean', ['string']);
    sim._getSimSpeed = module.cwrap('sim_get_sim_speed', 'number', []);
    return sim;
  }

  getSettingsJson() { return this._getSettingsJson(); }
  applySettingsJson(json) { return this._applySettingsJson(json); }
  get simSpeed() { return this._getSimSpeed ? this._getSimSpeed() : 1.0; }

  resize(numSlices, numLeds) { return this._simResize(numSlices, numLeds); }
  get numSlices() { return this._simNumSlices(); }
  get numLeds() { return this._simNumLeds(); }
  get patternCount() { return this._simNumPatterns(); }
  patternName(i) { return this._simPatternName(i); }

  setBrightness(b)    { this._simSetBrightness(b); }
  setColor(r, g, b)   { this._simSetColor(r, g, b); }
  setPhaseOffset(o)   { this._simSetPhaseOffset(o); }
  setText(t)          { this._simSetText(t); }
  setTextMode(m)      { this._simSetTextMode(m); }
  setTextDelay(ms)    { this._simSetTextDelay(ms); }
  setMirror(m)        { this._simSetMirror(m ? 1 : 0); }
  setRadialBalance(v) { this._simSetRadialBalance(v ? 1 : 0); }

  loadImage(rgbUint8Array, width, height) {
    const ptr = this._mod._malloc(rgbUint8Array.length);
    this._mod.HEAPU8.set(rgbUint8Array, ptr);
    const ok = this._simLoadImage(ptr, width, height);
    this._mod._free(ptr);
    return ok;
  }

  get animationCount() { return this._simNumAnimations(); }
  animationName(i)  { return this._simAnimationName(i); }
  animationKey(i)   { return this._simAnimationKey(i); }
  animParamCount(i) { return this._simAnimParamCount(i); }
  animParamKey(i, j)     { return this._simAnimParamKey(i, j); }
  animParamLabel(i, j)   { return this._simAnimParamLabel(i, j); }
  animParamValue(i, j)   { return this._simAnimParamValue(i, j); }
  animParamDefault(i, j) { return this._simAnimParamDefault(i, j); }
  animParamMin(i, j)     { return this._simAnimParamMin(i, j); }
  animParamMax(i, j)     { return this._simAnimParamMax(i, j); }
  animParamPresetCount(i, j)      { return this._simAnimParamPresetCount(i, j); }
  animParamPresetLabel(i, j, k)   { return this._simAnimParamPresetLabel(i, j, k); }
  animParamPresetValue(i, j, k)   { return this._simAnimParamPresetValue(i, j, k); }
  setAnimParam(i, j, v)  { this._simSetAnimParam(i, j, v); }

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
