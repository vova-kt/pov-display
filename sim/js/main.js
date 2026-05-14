import { PovSim } from './wasm-engine.js';

let sim;
let activePattern = 1;
let simTimeMs = 0;
let lastFrameTime = null;
let simSpeed = 1.0;
let paused = false;
let refreshRate = 30;
let numArms = 1;
let fpsFrameCount = 0;
let fpsLastTime = 0;
let currentFps = 0;

function sizeCanvas(canvas) {
  const dpr = window.devicePixelRatio || 1;
  const wrap = document.getElementById('canvas-wrap');
  const css = Math.max(300, Math.floor(Math.min(wrap.clientWidth, wrap.clientHeight)));
  canvas.style.width = css + 'px';
  canvas.style.height = css + 'px';
  canvas.width = css * dpr;
  canvas.height = css * dpr;
  if (sim) sim.rendererResize(css * dpr, css * dpr);
}

function readGeometry() {
  const numLeds   = +document.getElementById('num-leds').value;
  const ledSizeMm = +document.getElementById('led-size').value;
  const ledGapMm  = +document.getElementById('led-gap').value;
  const hubMm     = +document.getElementById('hub-radius').value;
  const pitchMm   = ledSizeMm + ledGapMm;
  const armMm     = hubMm + numLeds * pitchMm;
  return { numLeds, ledSizeMm, ledGapMm, hubMm, pitchMm, armMm };
}

function applyGeometry() {
  const g = readGeometry();
  sim.setGapFraction(g.ledGapMm / g.pitchMm);
  sim.setHubFraction(g.hubMm / g.armMm);
  document.getElementById('ro-pitch').textContent = g.pitchMm.toFixed(1) + ' mm';
  document.getElementById('ro-arm-radius').textContent = Math.round(g.armMm) + ' mm';
}

async function init() {
  const canvas = document.getElementById('pov-canvas');

  const numLeds = +document.getElementById('num-leds').value;
  const numSlices = +document.getElementById('num-slices').value;

  sim = await PovSim.create(numSlices, numLeds, canvas);

  sizeCanvas(canvas);
  refreshRate = +document.getElementById('refresh-rate').value;
  numArms = +document.getElementById('num-arms').value;
  sim.setNumArms(numArms);
  sim.setRpm(refreshRate * 60 / numArms);

  buildPatternSelector();
  bindControls();
  applyGeometry();

  window.addEventListener('resize', () => sizeCanvas(canvas));

  const gl = canvas.getContext('webgl2');
  console.log('GL context from JS:', gl ? 'exists' : 'null',
              'canvas size:', canvas.width, canvas.height,
              'drawingBufferWidth:', gl?.drawingBufferWidth,
              'drawingBufferHeight:', gl?.drawingBufferHeight);

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
    sim.resize(sim.numSlices, v);
    applyGeometry();
  });

  on('num-slices', 'change', e => {
    const v = clamp(+e.target.value, 36, 720);
    e.target.value = v;
    sim.resize(v, sim.numLeds);
  });

  on('phase-offset', 'input', e => {
    const v = +e.target.value;
    sim.setPhaseOffset(v);
    document.getElementById('phase-val').textContent = v + '\u00B0';
  });

  on('led-size', 'input', e => {
    document.getElementById('led-size-val').textContent = (+e.target.value).toFixed(1) + ' mm';
    applyGeometry();
  });
  on('led-gap', 'input', e => {
    document.getElementById('led-gap-val').textContent = (+e.target.value).toFixed(1) + ' mm';
    applyGeometry();
  });
  on('hub-radius', 'input', e => {
    document.getElementById('hub-radius-val').textContent = (+e.target.value) + ' mm';
    applyGeometry();
  });

  function applyRpm() {
    refreshRate = +document.getElementById('refresh-rate').value;
    numArms = +document.getElementById('num-arms').value;
    sim.setRpm(refreshRate * 60 / numArms);
    sim.setNumArms(numArms);
  }
  on('refresh-rate', 'change', applyRpm);
  on('num-arms', 'change', applyRpm);

  on('rpm-jitter', 'input', e => {
    sim.setRpmJitter(+e.target.value);
    document.getElementById('rpm-jitter-val').textContent = e.target.value + '%';
  });
  on('hall-jitter', 'input', e => {
    sim.setHallJitter(+e.target.value);
    document.getElementById('hall-jitter-val').textContent = e.target.value + ' \u00B5s';
  });
  on('hall-miss', 'input', e => {
    sim.setHallMiss(+e.target.value / 100);
    document.getElementById('hall-miss-val').textContent = e.target.value + '%';
  });
  on('timer-drift', 'input', e => {
    sim.setTimerDrift(+e.target.value);
    document.getElementById('timer-drift-val').textContent = e.target.value + ' ppm';
  });
  on('pattern-lag', 'input', e => {
    sim.setPatternLag(+e.target.value);
    document.getElementById('pattern-lag-val').textContent = e.target.value + ' ms';
  });
  on('spi-clock', 'input', e => {
    sim.setSpiClock(+e.target.value);
    document.getElementById('spi-clock-val').textContent = e.target.value + ' MHz';
  });

  on('display-hz', 'change', e => { sim.setDisplayHz(+e.target.value); });

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

  on('overlay-overruns', 'change', e => { sim.setShowOverruns(e.target.checked); });
  on('overlay-grid', 'change', e => { sim.setShowSliceGrid(e.target.checked); });
  on('overlay-hall', 'change', e => { sim.setShowHallMarker(e.target.checked); });
}

function loop(timestamp) {
  if (paused) return;

  if (lastFrameTime === null) lastFrameTime = timestamp;
  const dt = (timestamp - lastFrameTime) * simSpeed;
  lastFrameTime = timestamp;
  simTimeMs += dt;

  fpsFrameCount++;
  if (timestamp - fpsLastTime >= 1000) {
    currentFps = fpsFrameCount;
    fpsFrameCount = 0;
    fpsLastTime = timestamp;
  }

  sim.frame(dt, simTimeMs, activePattern);

  if (simTimeMs < 50) {
    const canvas = document.getElementById('pov-canvas');
    const gl = canvas.getContext('webgl2');
    if (gl) {
      const w = canvas.width, h = canvas.height;
      const spots = [
        ['center', w/2, h/2],
        ['mid-right', Math.round(w*0.75), h/2],
        ['mid-up', w/2, Math.round(h*0.75)],
        ['led-area', Math.round(w*0.65), h/2],
      ];
      for (const [name, x, y] of spots) {
        const px = new Uint8Array(4);
        gl.readPixels(x, y, 1, 1, gl.RGBA, gl.UNSIGNED_BYTE, px);
        console.log(name + ':', px[0], px[1], px[2], px[3]);
      }
    }
  }

  updateReadouts();
  requestAnimationFrame(loop);
}

function updateReadouts() {
  const actualRpm = sim.actualRpm;
  document.getElementById('ro-rpm').textContent = actualRpm.toFixed(0);
  document.getElementById('ro-eff-hz').textContent = (actualRpm * numArms / 60).toFixed(1) + ' Hz';
  document.getElementById('ro-slice-interval').textContent = sim.sliceIntervalUs.toFixed(1);
  document.getElementById('ro-spi-time').textContent = sim.spiTransferUs.toFixed(1);

  const headroomEl = document.getElementById('ro-headroom');
  headroomEl.textContent = sim.headroomUs.toFixed(1);
  headroomEl.className = sim.headroomUs < 0 ? 'negative' : '';

  document.getElementById('ro-frame-age').textContent = sim.frameAge;
  document.getElementById('ro-pattern-gen').textContent = sim.patternGenMs.toFixed(1);
  document.getElementById('ro-hall-missed').textContent = sim.hallMissed ? 'YES' : 'no';

  document.getElementById('hud-rpm').textContent = actualRpm.toFixed(0);
  document.getElementById('hud-hz').textContent = (actualRpm * numArms / 60).toFixed(1);
  document.getElementById('hud-fps').textContent = currentFps;
}

function clamp(v, min, max) { return Math.max(min, Math.min(max, v)); }

init().catch(err => {
  document.body.innerHTML = '<pre style="color:red">' + err.message + '\n' + err.stack + '</pre>';
});
