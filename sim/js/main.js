import { PovSim } from './wasm-engine.js';
import { SettingsUI } from './settings_ui.js';

let sim;
let settingsUI;
let simTimeMs = 0;
let lastFrameTime = null;
let paused = false;
let accumulatedDt = 0;
let fpsFrameCount = 0;
let fpsLastTime = 0;
let currentFps = 0;
let lastRenderTimestamp = null;

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

async function init() {
  const canvas = document.getElementById('pov-canvas');

  sim = await PovSim.create(360, 40, canvas);
  sizeCanvas(canvas);

  // Build the settings UI from the WASM model
  const adapter = {
    getModel() {
      return Promise.resolve(JSON.parse(sim.getSettingsJson()));
    },
    apply(patch) {
      sim.applySettingsJson(JSON.stringify(patch));
      return Promise.resolve();
    }
  };
  settingsUI = await SettingsUI.init(adapter, document.getElementById('settings-root'));

  // Ad hoc: inject image upload into the image pattern's panel (sim-only).
  const imageIdx = (JSON.parse(sim.getSettingsJson()).patterns || []).find(p => p.key === 'image')?.index ?? -1;
  if (imageIdx >= 0) {
    const imgPanel = document.querySelector(`[data-pattern-index="${imageIdx}"]`);
    if (imgPanel) {
      const hiddenInp = document.createElement('input');
      hiddenInp.type = 'file';
      hiddenInp.accept = 'image/*';
      hiddenInp.style.display = 'none';

      const btn = document.createElement('button');
      btn.textContent = 'Choose image…';
      btn.addEventListener('click', () => hiddenInp.click());

      const row = document.createElement('div');
      row.className = 'setting-row';
      row.appendChild(hiddenInp);
      row.appendChild(btn);
      imgPanel.appendChild(row);

      hiddenInp.addEventListener('change', async e => {
        const file = e.target.files[0];
        if (!file) return;
        const sz = sim.numLeds * 2;
        const result = await preprocessImage(file, sz);
        sim.loadImage(result.pixels, result.width, result.height);
      });
    }
  }

  window.addEventListener('resize', () => sizeCanvas(canvas));
  requestAnimationFrame(loop);
}

function loop(timestamp) {
  if (paused) return;

  if (lastFrameTime === null) {
    lastFrameTime = timestamp;
    lastRenderTimestamp = timestamp;
  }

  const speed = sim.simSpeed;
  const dt = (timestamp - lastFrameTime) * speed;
  lastFrameTime = timestamp;
  simTimeMs += dt;
  accumulatedDt += dt;

  // Derive displayHz and numArms from the live model to drive render cadence.
  const model = JSON.parse(sim.getSettingsJson());
  const displayHz = findSettingValue(model, 'displayHz') ?? 120;
  const numArms = findSettingValue(model, 'numArms') ?? 2;
  const activePattern = model.activePattern ?? 0;

  const renderInterval = 1000.0 / displayHz;
  if (timestamp - lastRenderTimestamp >= renderInterval) {
    lastRenderTimestamp += renderInterval *
      Math.floor((timestamp - lastRenderTimestamp) / renderInterval);

    sim.frame(accumulatedDt, simTimeMs, activePattern);
    accumulatedDt = 0;

    fpsFrameCount++;
    if (timestamp - fpsLastTime >= 1000) {
      currentFps = fpsFrameCount;
      fpsFrameCount = 0;
      fpsLastTime = timestamp;
    }
    updateHud(numArms);
  }

  requestAnimationFrame(loop);
}

function findSettingValue(model, key) {
  for (const g of model.groups || []) {
    for (const section of g.sections || [])
      for (const s of section.settings || [])
        if (s.key === key) return s.value;
  }
  return null;
}

function updateHud(numArms) {
  const rpm = sim.actualRpm;
  document.getElementById('hud-rpm').textContent = rpm.toFixed(0);
  document.getElementById('hud-hz').textContent = (rpm * numArms / 60).toFixed(1);
  document.getElementById('hud-fps').textContent = currentFps;
  document.getElementById('hud-slice-int').textContent = sim.sliceIntervalUs.toFixed(1);
  document.getElementById('hud-spi').textContent = sim.spiTransferUs.toFixed(1);
  const headEl = document.getElementById('hud-headroom');
  headEl.textContent = sim.headroomUs.toFixed(1);
  headEl.style.color = sim.headroomUs < 0 ? '#f44' : '';
  document.getElementById('hud-frame-age').textContent = sim.frameAge;
  document.getElementById('hud-pattern-gen').textContent = sim.patternGenMs.toFixed(1);
  document.getElementById('hud-hall-missed').textContent = sim.hallMissed ? 'YES' : 'no';
}

init().catch(err => {
  document.body.innerHTML = '<pre style="color:red">' + err.message + '\n' + err.stack + '</pre>';
});
