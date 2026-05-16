#pragma once
#include <pgmspace.h>

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>POV Display</title>
<script src="/js/image-processor.js"></script>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:system-ui,sans-serif;background:#1a1a2e;color:#e0e0e0;padding:16px;max-width:480px;margin:0 auto}
h1{font-size:1.3em;margin-bottom:12px;color:#7fdbca}
.card{background:#16213e;border-radius:8px;padding:14px;margin-bottom:12px}
label{display:block;font-size:.85em;margin-bottom:4px;color:#999}
select,input[type=text],input[type=number]{width:100%;padding:8px;border:1px solid #333;border-radius:4px;background:#0f1a30;color:#e0e0e0;font-size:.9em;margin-bottom:10px}
input[type=range]{width:100%;margin-bottom:4px}
input[type=color]{width:60px;height:32px;border:none;background:none;cursor:pointer;margin-bottom:10px}
.val{font-size:.8em;color:#7fdbca;margin-bottom:10px;display:block}
button{padding:10px 16px;border:none;border-radius:4px;cursor:pointer;font-size:.9em;font-weight:600}
.btn-save{background:#00c853;color:#fff;width:100%;margin-top:8px}
.chk{display:flex;align-items:center;gap:6px;margin-bottom:10px;font-size:.85em}
.chk input{width:auto;margin:0}
.tab-bar{display:flex;gap:4px;margin-bottom:12px}
.tab-btn{padding:8px 16px;border:none;border-radius:4px;cursor:pointer;background:#0f1a30;color:#888;font-size:.85em}
.tab-btn.active{background:#2979ff;color:#fff}
.tab-panel{} .tab-panel.hidden{display:none}
.settings-section{margin-top:14px;padding-top:10px;border-top:1px solid #2b3b60}
.settings-section:first-child{margin-top:0}
.settings-section-title{margin-bottom:8px;color:#7fdbca;font-size:.75em;font-weight:700;text-transform:uppercase;letter-spacing:0}
.setting-row{margin-bottom:10px}
.setting-subrow label{padding-left:10px}
.effect-slot-params,.pattern-params{margin-bottom:8px}
.pattern-params{} .pattern-params.hidden{display:none}
.status{display:flex;gap:16px;flex-wrap:wrap;margin-bottom:12px}
.stat{text-align:center}
.stat .num{font-size:1.6em;font-weight:700;color:#7fdbca}
.stat .lbl{font-size:.75em;color:#888}
.hidden{display:none}
</style>
</head>
<body>
<h1>POV Display Control</h1>

<div class="status">
 <div class="stat"><div class="num" id="rpm">--</div><div class="lbl">RPM</div></div>
 <div class="stat"><div class="num" id="effHz">--</div><div class="lbl">Hz</div></div>
 <div class="stat"><div class="num" id="heap">--</div><div class="lbl">Free KB</div></div>
</div>

<input type="file" id="imageFile" accept="image/*" style="margin-bottom:12px;font-size:.85em">
<div id="settings-root"></div>
<button class="btn-save" onclick="save()">Save to Flash</button>

<script type="module">
import { SettingsUI } from '/js/settings.js';

const adapter = {
  async getModel() {
    const r = await fetch('/api/config');
    return r.json();
  },
  async apply(patch) {
    await fetch('/api/config', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify(patch)
    });
  }
};

let ui;
SettingsUI.init(adapter, document.getElementById('settings-root'))
  .then(instance => { ui = instance; })
  .catch(err => console.error('settings init error:', err));

async function save() {
  await fetch('/api/save', {method: 'POST'});
}
window.save = save;

// Image upload — out-of-band, not via settings registry
const imageEl = document.getElementById('imageFile');
imageEl.addEventListener('change', async e => {
  const file = e.target.files[0];
  if (!file) return;
  const numLeds = 26; // sensible default; server triggers resize via configCb
  const sz = numLeds * 2;
  const result = await preprocessImage(file, sz);
  await fetch('/api/image?w=' + result.width + '&h=' + result.height, {
    method: 'POST',
    headers: {'Content-Type': 'application/octet-stream'},
    body: result.pixels
  });
  if (ui) await ui.load(); // refresh model (activePattern changed server-side)
});

// Status polling
setInterval(async () => {
  try {
    const s = await (await fetch('/api/status')).json();
    document.getElementById('rpm').textContent = s.rpm;
    const arms = 2; // default; accurate enough for status display
    document.getElementById('effHz').textContent = (s.rpm * arms / 60).toFixed(1);
    document.getElementById('heap').textContent = Math.round(s.freeHeap / 1024);
  } catch(_) {}
}, 1000);
</script>
</body>
</html>
)rawliteral";
