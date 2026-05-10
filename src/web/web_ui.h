#pragma once
#include <pgmspace.h>

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>POV Display</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:system-ui,sans-serif;background:#1a1a2e;color:#e0e0e0;padding:16px;max-width:480px;margin:0 auto}
h1{font-size:1.3em;margin-bottom:12px;color:#7fdbca}
.card{background:#16213e;border-radius:8px;padding:14px;margin-bottom:12px}
.card h2{font-size:.95em;color:#64b5f6;margin-bottom:10px}
label{display:block;font-size:.85em;margin-bottom:4px;color:#999}
select,input[type=text],input[type=number]{width:100%;padding:8px;border:1px solid #333;border-radius:4px;background:#0f1a30;color:#e0e0e0;font-size:.9em;margin-bottom:10px}
input[type=range]{width:100%;margin-bottom:4px}
input[type=color]{width:60px;height:32px;border:none;background:none;cursor:pointer;margin-bottom:10px}
.val{font-size:.8em;color:#7fdbca;margin-bottom:10px;display:block}
.row{display:flex;gap:10px;align-items:flex-end}
.row>*{flex:1}
button{padding:10px 16px;border:none;border-radius:4px;cursor:pointer;font-size:.9em;font-weight:600}
.btn-apply{background:#2979ff;color:#fff;width:100%}
.btn-save{background:#00c853;color:#fff;width:100%}
.btn-stop{background:#ff1744;color:#fff}
.status{display:flex;gap:16px;flex-wrap:wrap}
.stat{text-align:center}
.stat .num{font-size:1.6em;font-weight:700;color:#7fdbca}
.stat .lbl{font-size:.75em;color:#888}
.btns{display:flex;gap:8px;margin-top:8px}
.btns>*{flex:1}
</style>
</head>
<body>
<h1>POV Display Control</h1>

<div class="card">
 <h2>Status</h2>
 <div class="status">
  <div class="stat"><div class="num" id="rpm">--</div><div class="lbl">RPM</div></div>
  <div class="stat"><div class="num" id="heap">--</div><div class="lbl">Free KB</div></div>
 </div>
</div>

<div class="card">
 <h2>Pattern</h2>
 <label>Type</label>
 <select id="pattern">
  <option value="0">Solid Color</option>
  <option value="1">Rainbow</option>
  <option value="2">Text</option>
 </select>

 <label>Color</label>
 <input type="color" id="color" value="#ff0000">

 <label>Text (for text pattern)</label>
 <input type="text" id="text" maxlength="63" value="HELLO">
</div>

<div class="card">
 <h2>Display</h2>
 <label>Brightness (<span id="brVal">16</span>/31)</label>
 <input type="range" id="brightness" min="0" max="31" value="16" oninput="$('#brVal').textContent=this.value">

 <div class="row">
  <div>
   <label>LEDs</label>
   <input type="number" id="numLeds" min="1" max="144" value="36">
  </div>
  <div>
   <label>Slices</label>
   <input type="number" id="numSlices" min="36" max="720" value="360">
  </div>
 </div>

 <label>Phase Offset (<span id="phVal">0</span>)</label>
 <input type="range" id="phaseOffset" min="-180" max="180" value="0" oninput="$('#phVal').textContent=this.value">
</div>

<div class="card">
 <h2>Motor</h2>
 <label>ESC Pulse (<span id="escVal">1000</span> &micro;s)</label>
 <input type="range" id="escPulse" min="1000" max="2000" value="1000" oninput="$('#escVal').textContent=this.value">
 <button class="btn-stop" onclick="stopMotor()">STOP MOTOR</button>
</div>

<div class="btns">
 <button class="btn-apply" onclick="apply()">Apply</button>
 <button class="btn-save" onclick="save()">Save to Flash</button>
</div>

<script>
const $=s=>document.querySelector(s);

function rgbToHex(r,g,b){return '#'+[r,g,b].map(x=>x.toString(16).padStart(2,'0')).join('');}
function hexToRgb(h){const m=h.match(/\w\w/g);return m?m.map(x=>parseInt(x,16)):[255,0,0];}

async function loadConfig(){
 try{
  const r=await fetch('/api/config');
  const c=await r.json();
  $('#pattern').value=c.activePattern;
  $('#color').value=rgbToHex(c.colorR,c.colorG,c.colorB);
  $('#text').value=c.text;
  $('#brightness').value=c.brightness;$('#brVal').textContent=c.brightness;
  $('#numLeds').value=c.numLeds;
  $('#numSlices').value=c.numSlices;
  $('#phaseOffset').value=c.phaseOffset;$('#phVal').textContent=c.phaseOffset;
  $('#escPulse').value=c.escPulseUs;$('#escVal').textContent=c.escPulseUs;
 }catch(e){console.error(e);}
}

async function apply(){
 const rgb=hexToRgb($('#color').value);
 const body={
  activePattern:+$('#pattern').value,
  colorR:rgb[0],colorG:rgb[1],colorB:rgb[2],
  text:$('#text').value,
  brightness:+$('#brightness').value,
  numLeds:+$('#numLeds').value,
  numSlices:+$('#numSlices').value,
  phaseOffset:+$('#phaseOffset').value,
  escPulseUs:+$('#escPulse').value
 };
 await fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});
}

async function save(){
 await apply();
 await fetch('/api/save',{method:'POST'});
}

function stopMotor(){
 $('#escPulse').value=1000;$('#escVal').textContent='1000';
 apply();
}

async function pollStatus(){
 try{
  const r=await fetch('/api/status');
  const s=await r.json();
  $('#rpm').textContent=s.rpm;
  $('#heap').textContent=Math.round(s.freeHeap/1024);
 }catch(e){}
}

loadConfig();
setInterval(pollStatus,1000);
</script>
</body>
</html>
)rawliteral";
