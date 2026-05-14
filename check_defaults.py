#!/usr/bin/env python3
"""Verify config defaults haven't diverged from src/config.h (the source of truth).

Several files duplicate default values from the Config struct (HTML forms,
JS variables, C++ timing structs). This script detects when they drift apart.

Run:  python3 check_defaults.py
"""
import re, sys, os

ROOT = os.path.dirname(os.path.abspath(__file__))


def read(relpath):
    with open(os.path.join(ROOT, relpath)) as f:
        return f.read()


def parse_config_h():
    text = read('src/config.h')
    m = re.search(r'struct Config\s*\{(.+?)\n\s*void\b', text, re.DOTALL)
    if not m:
        sys.exit("PARSE ERROR: Could not find Config struct in src/config.h")
    body = m.group(1)

    defaults = {}
    for m in re.finditer(
        r'^\s+\w[\w:]+\s+(\w+)\s*(?:\[\d+\])?\s*=\s*("(?:[^"\\]|\\.)*"|[\w.+-]+)',
        body, re.MULTILINE
    ):
        name, raw = m.group(1), m.group(2).strip()
        if raw.startswith('"'):
            defaults[name] = raw[1:-1]
        elif '.' in raw:
            defaults[name] = float(raw.rstrip('f'))
        else:
            defaults[name] = int(raw)
    return defaults


def mismatch(errors, file, field, expected, actual):
    if actual is None:
        errors.append(f"  {file}: {field} — not found (expected {expected})")
    elif str(expected) != str(actual):
        errors.append(f"  {file}: {field} = {actual} (expected {expected})")


def extract_input_val(html, id_attr):
    m = re.search(rf'id="{id_attr}"[^>]*\bvalue="([^"]*)"', html)
    return m.group(1) if m else None


def extract_selected_option(html, id_attr):
    m = re.search(rf'<select\s+id="{id_attr}"[^>]*>(.+?)</select>', html, re.DOTALL)
    if not m:
        return None
    m2 = re.search(r'<option\s+value="([^"]*)"[^>]*\bselected\b', m.group(1))
    return m2.group(1) if m2 else None


def extract_cpp_field(struct_body, name):
    m = re.search(rf'{name}\s*=\s*([^;]+);', struct_body)
    if not m:
        return None
    raw = m.group(1).strip().rstrip('f')
    raw = re.sub(r'//.*', '', raw).strip()
    try:
        return float(raw) if '.' in raw else int(raw)
    except ValueError:
        return raw


def check_timing_h(d):
    errors = []
    text = read('sim/timing.h')
    m = re.search(r'struct TimingState\s*\{(.+?)\};', text, re.DOTALL)
    if not m:
        sys.exit("PARSE ERROR: Could not find TimingState in sim/timing.h")
    body = m.group(1)

    mismatch(errors, 'sim/timing.h', 'numLeds',
             d['numLeds'], extract_cpp_field(body, 'numLeds'))
    mismatch(errors, 'sim/timing.h', 'numSlices',
             d['numSlices'], extract_cpp_field(body, 'numSlices'))
    mismatch(errors, 'sim/timing.h', 'spiClockMhz',
             float(d['spiClockMhz']), extract_cpp_field(body, 'spiClockMhz'))
    mismatch(errors, 'sim/timing.h', 'patternFps (=targetHz)',
             float(d['targetHz']), extract_cpp_field(body, 'patternFps'))

    expected_rpm = float(d['targetHz']) * 60.0 / float(d['numArms'])
    mismatch(errors, 'sim/timing.h', 'rpm (=targetHz*60/numArms)',
             expected_rpm, extract_cpp_field(body, 'rpm'))
    return errors


def check_sim_html(d):
    errors = []
    html = read('sim/index.html')

    mismatch(errors, 'sim/index.html', 'num-leds',
             str(d['numLeds']), extract_input_val(html, 'num-leds'))
    mismatch(errors, 'sim/index.html', 'brightness',
             str(d['brightness']), extract_input_val(html, 'brightness'))
    mismatch(errors, 'sim/index.html', 'text',
             d['text'], extract_input_val(html, 'text'))
    mismatch(errors, 'sim/index.html', 'refresh-rate',
             str(d['targetHz']), extract_selected_option(html, 'refresh-rate'))
    mismatch(errors, 'sim/index.html', 'num-arms',
             str(d['numArms']), extract_selected_option(html, 'num-arms'))
    mismatch(errors, 'sim/index.html', 'spi-clock',
             str(d['spiClockMhz']), extract_selected_option(html, 'spi-clock'))

    expected_hex = f"#{d['colorR']:02x}{d['colorG']:02x}{d['colorB']:02x}"
    mismatch(errors, 'sim/index.html', 'color',
             expected_hex, extract_input_val(html, 'color'))

    expected_phase = str((d['phaseOffset'] + 90 + 360) % 360)
    mismatch(errors, 'sim/index.html', 'phase-offset',
             expected_phase, extract_selected_option(html, 'phase-offset'))
    return errors


def check_sim_js(d):
    errors = []
    text = read('sim/js/main.js')

    def js_let(name):
        m = re.search(rf'let\s+{name}\s*=\s*(\S+?)\s*;', text)
        if not m:
            return None
        try:
            return int(m.group(1))
        except ValueError:
            return m.group(1)

    mismatch(errors, 'sim/js/main.js', 'activePattern',
             d['activePattern'], js_let('activePattern'))
    mismatch(errors, 'sim/js/main.js', 'refreshRate (=targetHz)',
             d['targetHz'], js_let('refreshRate'))
    mismatch(errors, 'sim/js/main.js', 'numArms',
             d['numArms'], js_let('numArms'))
    return errors


def check_web_ui(d):
    errors = []
    html = read('src/web/web_ui.h')

    mismatch(errors, 'src/web/web_ui.h', 'numLeds',
             str(d['numLeds']), extract_input_val(html, 'numLeds'))
    mismatch(errors, 'src/web/web_ui.h', 'text',
             d['text'], extract_input_val(html, 'text'))
    mismatch(errors, 'src/web/web_ui.h', 'brightness',
             str(d['brightness']), extract_input_val(html, 'brightness'))

    expected_hex = f"#{d['colorR']:02x}{d['colorG']:02x}{d['colorB']:02x}"
    mismatch(errors, 'src/web/web_ui.h', 'color',
             expected_hex, extract_input_val(html, 'color'))
    return errors


def check_config_cpp(d):
    errors = []
    text = read('src/config.cpp')
    m = re.search(r'strcpy\(text,\s*"([^"]*)"\)', text)
    if m:
        mismatch(errors, 'src/config.cpp', 'text fallback',
                 d['text'], m.group(1))
    return errors


def main():
    defaults = parse_config_h()

    errors = []
    errors += check_timing_h(defaults)
    errors += check_sim_html(defaults)
    errors += check_sim_js(defaults)
    errors += check_web_ui(defaults)
    errors += check_config_cpp(defaults)

    if errors:
        print("Config defaults diverged from src/config.h:\n")
        for e in errors:
            print(e)
        print(f"\n{len(errors)} mismatch(es). Update consumer files to match src/config.h.")
        return 1

    print(f"All config defaults consistent ({len(defaults)} fields checked across 5 files).")
    return 0


if __name__ == '__main__':
    sys.exit(main())
