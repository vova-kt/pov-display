#pragma once
#include <pgmspace.h>

// Auto-generated from sim/js/settings_ui.js — do not edit directly.
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

  }

  _switchTab(key) {
    localStorage.setItem('pov_settings_tab', key);
    for (const el of this._root.querySelectorAll('.tab-btn'))
      el.classList.toggle('active', el.dataset.key === key);
    for (const el of this._root.querySelectorAll('.tab-panel'))
      el.classList.toggle('hidden', el.dataset.key !== key);
  }

  _renderGroup(panel, group) {
    for (const section of group.sections || []) {
      const sectionEl = el('section', 'settings-section');
      sectionEl.dataset.section = section.key;

      const title = el('div', 'settings-section-title');
      title.textContent = section.label;
      sectionEl.appendChild(title);

      for (const s of section.settings || []) {
        sectionEl.appendChild(this._makeSettingRow(s));
      }

      if (group.key === 'picture' && section.key === 'pattern')
        this._renderPatternPanels(sectionEl);
      if (group.key === 'picture' && section.key === 'animations')
        this._renderAnimationStack(sectionEl);

      panel.appendChild(sectionEl);
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

  _renderAnimationStack(panel) {
    const stack = this._model.animationStack || [];
    const slotCount = stack.length || 2;
    for (let slot = 0; slot < slotCount; slot++) {
      const row = el('div', 'setting-row');
      row.appendChild(label('Animation ' + (slot + 1)));

      const sel = el('select');
      const none = el('option');
      none.value = '';
      none.textContent = 'None';
      sel.appendChild(none);

      for (const anim of this._model.animations || []) {
        const opt = el('option');
        opt.value = anim.key;
        opt.textContent = anim.name;
        sel.appendChild(opt);
      }

      sel.value = stack[slot] ?? '';
      sel.addEventListener('change', () => this._changeAnimationSlot(slot, sel.value));
      row.appendChild(sel);
      panel.appendChild(row);

      const paramsWrap = el('div', 'animation-slot-params');
      paramsWrap.dataset.slot = slot;
      for (const anim of this._model.animations || []) {
        const selected = anim.key === (stack[slot] ?? '');
        const div = el('div', 'animation-params' + (selected ? '' : ' hidden'));
        div.dataset.animKey = anim.key;
        for (const p of anim.params || [])
          div.appendChild(this._makeNestedParamRow(p, 'animations', anim.key));
        paramsWrap.appendChild(div);
      }
      panel.appendChild(paramsWrap);
    }
  }

  _renderPatternPanels(panel) {
    for (const pat of this._model.patterns || []) {
      const div = el('div', 'pattern-params' + (pat.index === this._activePattern ? '' : ' hidden'));
      div.dataset.patternIndex = pat.index;
      for (const p of pat.params || []) {
        div.appendChild(this._makeNestedParamRow(p, 'patterns', pat.key));
      }
      panel.appendChild(div);
    }
  }

  _makeNestedParamRow(p, section, parentKey) {
    const row = el('div', 'setting-row setting-subrow');
    row.appendChild(label(p.label));

    if (p.type === 'text') {
      const inp = el('input');
      inp.type = 'text';
      inp.value = p.value ?? '';
      inp.maxLength = 63;
      let debounce;
      inp.addEventListener('input', () => {
        clearTimeout(debounce);
        debounce = setTimeout(() =>
          this._changeNested(section, parentKey, p.key, inp.value), 400);
      });
      inp.addEventListener('blur', () =>
        this._changeNested(section, parentKey, p.key, inp.value));
      row.appendChild(inp);

    } else if (p.type === 'enum' && p.options?.length) {
      const sel = el('select');
      for (const [optLabel, optVal] of p.options) {
        const opt = el('option');
        opt.value = optVal;
        opt.textContent = optLabel;
        sel.appendChild(opt);
      }
      sel.value = p.value;
      sel.addEventListener('change', () =>
        this._changeNested(section, parentKey, p.key, +sel.value, true));
      row.appendChild(sel);

    } else if (p.type === 'bool') {
      const cb = el('input');
      cb.type = 'checkbox';
      cb.checked = p.value !== 0;
      cb.addEventListener('change', () =>
        this._changeNested(section, parentKey, p.key, cb.checked ? 1 : 0, true));
      row.appendChild(cb);

    } else if (p.type === 'color') {
      const inp = el('input');
      inp.type = 'color';
      inp.value = '#' + ((p.value >>> 0) & 0xFFFFFF).toString(16).padStart(6, '0');
      inp.addEventListener('input', () =>
        this._changeNested(section, parentKey, p.key, parseInt(inp.value.slice(1), 16)));
      row.appendChild(inp);

    } else {
      const inp = el('input');
      inp.type = 'range';
      inp.min = p.min;
      inp.max = p.max;
      inp.value = p.value;
      const valSpan = el('span', 'val');
      valSpan.textContent = p.value;
      inp.addEventListener('input', () => {
        valSpan.textContent = inp.value;
        this._changeNested(section, parentKey, p.key, +inp.value);
      });
      row.appendChild(inp);
      row.appendChild(valSpan);
    }

    return row;
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

  _changeAnimationSlot(slot, value) {
    if (!Array.isArray(this._model.animationStack))
      this._model.animationStack = [];
    this._model.animationStack[slot] = value;
    this._showAnimationSlotParams(slot, value);
    const slotCount = this._model.animationStack.length || 2;
    this._pending.animationStack = this._model.animationStack.slice(0, slotCount);
    this._flush();
  }

  _showAnimationSlotParams(slot, value) {
    const wrap = this._root.querySelector(`.animation-slot-params[data-slot="${slot}"]`);
    if (!wrap) return;
    for (const div of wrap.querySelectorAll('.animation-params'))
      div.classList.toggle('hidden', div.dataset.animKey !== value);
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
