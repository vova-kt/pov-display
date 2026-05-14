import { PovSim } from './wasm-engine.js';
import { TimingModel } from './timing-model.js';
import { RadialRenderer } from './radial-renderer.js';

let sim, timing, renderer;
let activePattern = 1;
let simTimeMs = 0;
let lastFrameTime = null;
let simSpeed = 1.0;
let paused = false;
let lastGenTimeMs = -Infinity;
let revolution = null;

async function init() {
  const canvas = document.getElementById('pov-canvas');
  canvas.width = 600;
  canvas.height = 600;

  sim = await PovSim.create(360, 144);
  timing = new TimingModel();
  renderer = new RadialRenderer(canvas);

  buildPatternSelector();
  bindControls();
  updateReadouts();

  requestAnimationFrame(loop);
}

function buildPatternSelector() {
  const sel = document.getElementById('pattern');
  sel.innerHTML = '';
  for (let i = 0; i < sim.patternCount; i++) {
    const opt = document.createElement('option');
    opt.value = i;
    opt.textContent = sim.patternName(i);
    sel.appendChild(opt);
  }
  sel.value = activePattern;
}

function bindControls() {
  const on = (id, evt, fn) => document.getElementById(id).addEventListener(evt, fn);

  on('pattern', 'change', e => { activePattern = +e.target.value; });

  on('color', 'input', e => {
    const hex = e.target.value;
    sim.setColor(
      parseInt(hex.slice(1, 3), 16),
      parseInt(hex.slice(3, 5), 16),
      parseInt(hex.slice(5, 7), 16)
    );
  });

  on('brightness', 'input', e => {
    const v = +e.target.value;
    sim.setBrightness(v);
    document.getElementById('brightness-val').textContent = v;
  });

  on('text', 'input', e => { sim.setText(e.target.value); });

  on('num-leds', 'change', e => {
    const v = clamp(+e.target.value, 1, 144);
    e.target.value = v;
    timing.numLeds = v;
    sim.resize(sim.numSlices, v);
    renderer.invalidateLookup();
  });

  on('num-slices', 'change', e => {
    const v = clamp(+e.target.value, 36, 720);
    e.target.value = v;
    timing.numSlices = v;
    sim.resize(v, sim.numLeds);
    renderer.invalidateLookup();
  });

  on('phase-offset', 'input', e => {
    const v = +e.target.value;
    sim.setPhaseOffset(v);
    document.getElementById('phase-val').textContent = v + '\u00B0';
  });

  on('rpm', 'input', e => {
    timing.rpm = +e.target.value;
    document.getElementById('rpm-val').textContent = timing.rpm;
  });
  on('rpm-jitter', 'input', e => {
    timing.rpmJitter = +e.target.value;
    document.getElementById('rpm-jitter-val').textContent = e.target.value + '%';
  });
  on('hall-jitter', 'input', e => {
    timing.hallJitterUs = +e.target.value;
    document.getElementById('hall-jitter-val').textContent = e.target.value + ' \u00B5s';
  });
  on('hall-miss', 'input', e => {
    timing.hallMissRate = +e.target.value / 100;
    document.getElementById('hall-miss-val').textContent = e.target.value + '%';
  });
  on('timer-drift', 'input', e => {
    timing.sliceTimerDriftPpm = +e.target.value;
    document.getElementById('timer-drift-val').textContent = e.target.value + ' ppm';
  });
  on('pattern-lag', 'input', e => {
    timing.patternLagMs = +e.target.value;
    document.getElementById('pattern-lag-val').textContent = e.target.value + ' ms';
  });
  on('spi-clock', 'input', e => {
    timing.spiClockMhz = +e.target.value;
    document.getElementById('spi-clock-val').textContent = e.target.value + ' MHz';
  });

  on('sim-speed', 'input', e => {
    simSpeed = +e.target.value;
    document.getElementById('sim-speed-val').textContent = simSpeed.toFixed(1) + 'x';
  });
  on('pause-btn', 'click', () => {
    paused = !paused;
    document.getElementById('pause-btn').textContent = paused ? 'Resume' : 'Pause';
    if (!paused) {
      lastFrameTime = null;
      requestAnimationFrame(loop);
    }
  });

  on('overlay-overruns', 'change', e => { renderer.showOverruns = e.target.checked; });
  on('overlay-grid', 'change', e => { renderer.showSliceGrid = e.target.checked; });
  on('overlay-hall', 'change', e => { renderer.showHallMarker = e.target.checked; });
}

function loop(timestamp) {
  if (paused) return;

  if (lastFrameTime === null) lastFrameTime = timestamp;
  const dt = (timestamp - lastFrameTime) * simSpeed;
  lastFrameTime = timestamp;
  simTimeMs += dt;

  const patternInterval = 1000 / timing.patternFps;
  if (simTimeMs - lastGenTimeMs >= patternInterval) {
    sim.generate(activePattern, simTimeMs | 0);
    lastGenTimeMs = simTimeMs;
  }

  revolution = timing.simulateRevolution(simTimeMs);

  const phaseOffset = +document.getElementById('phase-offset').value;
  renderer.render(sim, revolution, phaseOffset);

  updateReadouts();
  requestAnimationFrame(loop);
}

function updateReadouts() {
  const r = revolution;
  if (!r) return;

  document.getElementById('ro-rpm').textContent = r.actualRpm.toFixed(0);
  document.getElementById('ro-slice-interval').textContent = r.sliceIntervalUs.toFixed(1);
  document.getElementById('ro-spi-time').textContent = r.spiTransferUs.toFixed(1);

  const headroomEl = document.getElementById('ro-headroom');
  headroomEl.textContent = r.headroomUs.toFixed(1);
  headroomEl.className = r.headroomUs < 0 ? 'negative' : '';

  document.getElementById('ro-frame-age').textContent = r.frameAge;
  document.getElementById('ro-pattern-gen').textContent = timing.patternGenTimeMs().toFixed(1);
  document.getElementById('ro-hall-missed').textContent = r.hallMissed ? 'YES' : 'no';
}

function clamp(v, min, max) { return Math.max(min, Math.min(max, v)); }

init().catch(err => {
  document.body.innerHTML = '<pre style="color:red">' + err.message + '\n' + err.stack + '</pre>';
});
