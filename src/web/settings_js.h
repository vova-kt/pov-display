#pragma once
#include <pgmspace.h>

// Auto-generated from js/settings_ui.js — do not edit directly.
// Run tools/gen_embedded_js.sh to regenerate.

static const char SETTINGS_JS[] PROGMEM = R"rawliteral(
// Shared settings renderer — used by both the sim and the MCU web UI.
// Adapter interface: { getModel() → Promise<model>, apply(patch) → Promise<void> }
export class SettingsUI {
  constructor(adapter, rootEl) {
    this._adapter = adapter;
    this._root = rootEl;
    this._model = null;
    this._activePattern = 0;
    this._pending = {};
    this._debounceTimer = null;
  }

  static async init(adapter, rootEl) {
    const ui = new SettingsUI(adapter, rootEl);
    await ui.load();
    return ui;
  }

  get activePattern() { return this._activePattern; }

  async load() {
    this._model = await this._adapter.getModel();
    this._activePattern = this._model.activePattern ?? 0;
    this._render();
  }

  _render() {
    this._root.innerHTML = '';
    const tabs = this._model.groups || [];
    const defaultKey = tabs[0]?.key ?? 'picture';
    const savedTab = localStorage.getItem('pov_settings_tab') ?? defaultKey;

    // Tab bar
    const bar = el('div', 'tab-bar');
    this._root.appendChild(bar);

    // Tab panels
    const panels = {};
    for (const group of tabs) {
      const btn = el('button', 'tab-btn' + (group.key === savedTab ? ' active' : ''));
      btn.textContent = group.label;
      btn.dataset.key = group.key;
      btn.addEventListener('click', () => this._switchTab(group.key));
      bar.appendChild(btn);

      const panel = el('div', 'tab-panel' + (group.key === savedTab ? '' : ' hidden'));
      panel.dataset.key = group.key;
      this._root.appendChild(panel);
      panels[group.key] = panel;

      this._renderGroup(panel, group);
    }

    // Pattern + animation panels go into the picture group panel.
    const picPanel = panels['picture'];
    if (picPanel) {
      this._renderAnimations(picPanel);
      this._renderPatternPanels(picPanel);
    }
  }

  _switchTab(key) {
    localStorage.setItem('pov_settings_tab', key);
    for (const el of this._root.querySelectorAll('.tab-btn'))
      el.classList.toggle('active', el.dataset.key === key);
    for (const el of this._root.querySelectorAll('.tab-panel'))
      el.classList.toggle('hidden', el.dataset.key !== key);
  }

  _renderGroup(panel, group) {
    for (const s of group.settings || []) {
      panel.appendChild(this._makeSettingRow(s));
    }
  }

  _makeSettingRow(s) {
    const wrap = el('div', 'setting-row');
    wrap.dataset.key = s.key;

    const scale = s.scale > 1 ? s.scale : 1;

    if (s.type === 'bool') {
      const label = el('label', 'chk');
      const cb = el('input');
      cb.type = 'checkbox';
      cb.checked = s.value !== 0;
      cb.addEventListener('change', () => this._change(s.key, cb.checked ? 1 : 0));
      label.appendChild(cb);
      label.appendChild(text(' ' + s.label));
      wrap.appendChild(label);

    } else if (s.type === 'color') {
      wrap.appendChild(label(s.label));
      const inp = el('input');
      inp.type = 'color';
      inp.value = '#' + ((s.value >>> 0) & 0xFFFFFF).toString(16).padStart(6, '0');
      inp.addEventListener('input', () =>
        this._change(s.key, parseInt(inp.value.slice(1), 16)));
      wrap.appendChild(inp);

    } else if (s.type === 'enum') {
      wrap.appendChild(label(s.label));
      const sel = el('select');
      for (const [optLabel, optVal] of (s.options || [])) {
        const opt = el('option');
        opt.value = optVal;
        opt.textContent = optLabel;
        sel.appendChild(opt);
      }
      sel.value = s.value;
      sel.addEventListener('change', () => {
        const v = +sel.value;
        if (s.key === 'activePattern') this._changeActivePattern(v);
        else this._change(s.key, v);
      });
      wrap.appendChild(sel);

    } else if (s.type === 'int') {
      wrap.appendChild(label(s.label));
      if (s.options && s.options.length) {
        // Int with preset options → select
        const sel = el('select');
        for (const [optLabel, optVal] of s.options) {
          const opt = el('option');
          opt.value = optVal;
          opt.textContent = optLabel;
          sel.appendChild(opt);
        }
        sel.value = s.value;
        sel.addEventListener('change', () => this._change(s.key, +sel.value));
        wrap.appendChild(sel);
      } else {
        const dispMin = s.min / scale;
        const dispMax = s.max / scale;
        const dispVal = s.value / scale;
        const inp = el('input');
        inp.type = 'range';
        inp.min = dispMin;
        inp.max = dispMax;
        inp.step = scale > 1 ? (1 / scale) : 1;
        inp.value = dispVal;
        const valSpan = el('span', 'val');
        valSpan.textContent = scale > 1 ? dispVal.toFixed(1) : dispVal;
        inp.addEventListener('input', () => {
          const raw = Math.round(+inp.value * scale);
          valSpan.textContent = scale > 1 ? (+inp.value).toFixed(1) : raw;
          this._change(s.key, raw);
        });
        wrap.appendChild(inp);
        wrap.appendChild(valSpan);
      }

    } else if (s.type === 'text') {
      wrap.appendChild(label(s.label));
      const inp = el('input');
      inp.type = 'text';
      inp.value = s.value ?? '';
      inp.maxLength = 63;
      let debounce;
      inp.addEventListener('input', () => {
        clearTimeout(debounce);
        debounce = setTimeout(() => this._changeText(s.key, inp.value), 400);
      });
      inp.addEventListener('blur', () => this._changeText(s.key, inp.value));
      wrap.appendChild(inp);
    }

    return wrap;
  }

  _renderAnimations(panel) {
    for (const anim of this._model.animations || []) {
      for (const p of anim.params || []) {
        const row = el('div', 'setting-row');
        const labelText = anim.name + ': ' + p.label;
        if (p.type === 'enum' && p.options?.length) {
          row.appendChild(label(labelText));
          const sel = el('select');
          for (const [optLabel, optVal] of p.options) {
            const opt = el('option');
            opt.value = optVal;
            opt.textContent = optLabel;
            sel.appendChild(opt);
          }
          sel.value = p.value;
          const animKey = anim.key, paramKey = p.key;
          sel.addEventListener('change', () =>
            this._changeNested('animations', animKey, paramKey, +sel.value));
          row.appendChild(sel);
        } else {
          row.appendChild(label(labelText));
          const inp = el('input');
          inp.type = 'range';
          inp.min = p.min; inp.max = p.max; inp.value = p.value;
          const valSpan = el('span', 'val');
          valSpan.textContent = p.value;
          const animKey = anim.key, paramKey = p.key;
          inp.addEventListener('input', () => {
            valSpan.textContent = inp.value;
            this._changeNested('animations', animKey, paramKey, +inp.value);
          });
          row.appendChild(inp);
          row.appendChild(valSpan);
        }
        panel.appendChild(row);
      }
    }
  }

  _renderPatternPanels(panel) {
    for (const pat of this._model.patterns || []) {
      const div = el('div', 'pattern-params' + (pat.index === this._activePattern ? '' : ' hidden'));
      div.dataset.patternIndex = pat.index;
      for (const p of pat.params || []) {
        const row = el('div', 'setting-row');
        if (p.type === 'text') {
          row.appendChild(label(p.label));
          const inp = el('input');
          inp.type = 'text';
          inp.value = p.value ?? '';
          inp.maxLength = 63;
          let debounce;
          const patKey = pat.key, paramKey = p.key;
          inp.addEventListener('input', () => {
            clearTimeout(debounce);
            debounce = setTimeout(() =>
              this._changeNested('patterns', patKey, paramKey, inp.value), 400);
          });
          inp.addEventListener('blur', () =>
            this._changeNested('patterns', pat.key, p.key, inp.value));
          row.appendChild(inp);
        } else if (p.type === 'enum') {
          row.appendChild(label(p.label));
          const sel = el('select');
          for (const [optLabel, optVal] of (p.options || [])) {
            const opt = el('option');
            opt.value = optVal;
            opt.textContent = optLabel;
            sel.appendChild(opt);
          }
          sel.value = p.value;
          const patKey = pat.key, paramKey = p.key;
          sel.addEventListener('change', () =>
            this._changeNested('patterns', patKey, paramKey, +sel.value, true));
          row.appendChild(sel);
        } else {
          row.appendChild(label(p.label));
          const inp = el('input');
          inp.type = 'range';
          inp.min = p.min; inp.max = p.max; inp.value = p.value;
          const valSpan = el('span', 'val');
          valSpan.textContent = p.value;
          const patKey = pat.key, paramKey = p.key;
          inp.addEventListener('input', () => {
            valSpan.textContent = inp.value;
            this._changeNested('patterns', patKey, paramKey, +inp.value);
          });
          row.appendChild(inp);
          row.appendChild(valSpan);
        }
        div.appendChild(row);
      }
      panel.appendChild(div);
    }
  }

  _changeActivePattern(v) {
    this._activePattern = v;
    // Show only the active pattern's params panel.
    for (const div of this._root.querySelectorAll('.pattern-params'))
      div.classList.toggle('hidden', +div.dataset.patternIndex !== v);
    this._change('activePattern', v);
  }

  _change(key, value) {
    if (!this._pending.settings) this._pending.settings = {};
    this._pending.settings[key] = value;
    if (key === 'activePattern') this._pending.activePattern = value;
    this._flush();
  }

  _changeText(key, value) {
    if (!this._pending.settings) this._pending.settings = {};
    this._pending.settings[key] = value;
    this._flush();
  }

  _changeNested(section, parentKey, paramKey, value, immediate = false) {
    if (!this._pending[section]) this._pending[section] = {};
    if (!this._pending[section][parentKey]) this._pending[section][parentKey] = {};
    this._pending[section][parentKey][paramKey] = value;
    if (immediate) this._flush();
    else this._flush();
  }

  _flush() {
    clearTimeout(this._debounceTimer);
    this._debounceTimer = setTimeout(() => {
      const patch = this._pending;
      this._pending = {};
      this._adapter.apply(patch).catch(e => console.error('settings apply error:', e));
    }, 50);
  }
}

// ── DOM helpers ────────────────────────────────────────────────────────────

function el(tag, cls) {
  const e = document.createElement(tag);
  if (cls) e.className = cls;
  return e;
}
function text(s) {
  return document.createTextNode(s);
}
function label(s) {
  const l = el('label');
  l.textContent = s;
  return l;
}
)rawliteral";
