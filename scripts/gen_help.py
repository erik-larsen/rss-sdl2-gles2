#!/usr/bin/env python3
"""gen_help.py - generate per-saver rss_help.cpp from handleCommandLine().

Each RSS saver lists its options as getArgumentsValue(argc, argv, "-name",
var, min, max) calls inside handleCommandLine(). This script parses those calls
and emits a small rss_help.cpp defining:

    const char* rss_saver_name    = "<saver>";
    const char* rss_saver_options = "  -name   <min..max>\n ...";

which the SDL shell (libs/librs) prints on --help. Savers with no options get
an empty table. Re-run after editing a saver's handleCommandLine.

Usage: gen_help.py savers/<name>/<name>.cpp [savers/<name2>/...]
       gen_help.py --all          (process every savers/*/ with a .cpp)
"""
import re
import sys
import os

# getArgumentsValue(argc, argv, std::string("-foo"), var, MIN, MAX)
#   capture: option name, and optional min, max (numeric or identifier)
CALL_RE = re.compile(
    r'getArgumentsValue\s*\(\s*argc\s*,\s*argv\s*,\s*'
    r'std::string\(\s*"(-[A-Za-z0-9_]+)"\s*\)\s*,\s*'
    r'([A-Za-z_][A-Za-z0-9_]*)\s*'
    r'(?:,\s*([A-Za-z0-9_]+)\s*,\s*([A-Za-z0-9_]+)\s*)?\)'
)

def parse_options(cpp_path):
    src = open(cpp_path, encoding='utf-8', errors='replace').read()
    # isolate the RS_XSCREENSAVER handleCommandLine body if present
    m = re.search(r'void\s+handleCommandLine\s*\([^)]*\)\s*\{', src)
    body = src[m.end():] if m else src
    # cut at the matching-ish end (next top-level function) - good enough:
    end = body.find('\n}\n')
    if end != -1:
        body = body[:end]
    opts = []
    seen = set()
    for mo in CALL_RE.finditer(body):
        name, var, lo, hi = mo.group(1), mo.group(2), mo.group(3), mo.group(4)
        if name in seen:
            continue
        seen.add(name)
        # Some savers read into a local temp and assign to the real global:
        #   int b;
        #   if(getArgumentsValue(..., "-fog", b, 0, 1) >= 0) dFog = b;
        # (cyclone, fieldlines, lattice). Follow the assignment so the
        # runtime table points at the global, not a function local.
        am = re.match(r'\s*>=\s*0\s*\)\s*([A-Za-z_]\w*)\s*=\s*' + re.escape(var) + r'\s*;',
                      body[mo.end():mo.end() + 120])
        if am:
            var = am.group(1)
        opts.append((name, var, lo, hi))
    return opts

def parse_defaults(cpp_path):
    """Default values, from setDefaults(). Two upstream shapes:
       - void setDefaults()      { dVar = N; ... }        (plain)
       - void setDefaults(int w) { switch(w){ case DEFAULTS1: dVar = N; ...
         (presets; handleCommandLine inits with DEFAULTS1, so use that block)
       Returns (defaults {var: number}, preset_names {n: label})."""
    src = open(cpp_path, encoding='utf-8', errors='replace').read()
    defaults, preset_names = {}, {}
    m = re.search(r'void\s+setDefaults\s*\(([^)]*)\)\s*\{', src)
    if not m:
        return defaults, preset_names
    body = src[m.end():]
    end = body.find('\n}\n')
    if end != -1:
        body = body[:end]
    if 'switch' in body and m.group(1).strip():
        for pm in re.finditer(r'case\s+DEFAULTS(\d+)\s*:\s*(?://\s*(.*))?', body):
            if pm.group(2):
                preset_names[int(pm.group(1))] = pm.group(2).strip()
        first_break = body.find('break;')
        if first_break != -1:
            body = body[:first_break]
    for am in re.finditer(r'\b([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(-?\d+(?:\.\d+)?)\s*;', body):
        defaults.setdefault(am.group(1), float(am.group(2)))
    return defaults, preset_names

def c_escape(s):
    return s.replace('\\', '\\\\').replace('"', '\\"')

def emit_runtime(opts):
    """Live-option runtime for the web settings panel: a name->address table,
    rss_set_option() to poke a value into the running saver, and
    rss_restart() (cleanUp + initSaver) so init-time options rebuild the
    scene in place. Preset options (-default) map to a handleCommandLine
    local, not a global, so they are excluded — the panel reloads for those.
    All option variables are int across the RSS savers."""
    live = [(n, v) for n, v, lo, hi in opts
            if not (re.match(r'DEFAULTS\d+', lo or '') and re.match(r'DEFAULTS\d+', hi or ''))]
    body = '\n/* ---- live-option runtime (web settings panel; see shell.html) ---- */\n'
    body += '#include <string.h>\n\n'
    if not live:
        body += ('extern "C" int rss_set_option(const char*, int) { return 0; }\n'
                 'extern "C" void rss_restart(void) {}\n')
        return body
    for var in sorted({v for _n, v in live}):
        body += 'extern int %s;\n' % var
    body += '\nstatic struct { const char* name; int* addr; int staged; int dirty; } rss_opts[] = {\n'
    for name, var in live:
        body += '    { "%s", &%s, 0, 0 },\n' % (c_escape(name.lstrip('-')), var)
    body += '};\n\n'
    # Values are STAGED here and only written into the real globals inside
    # rss_restart, between cleanUp() and initSaver(). Several savers' free
    # paths iterate over the option globals (solarwinds' wind::~wind loops
    # dParticles/dEmitters); writing a new count before cleanUp() makes it
    # free with the new count over arrays allocated with the old one.
    body += ('extern "C" int rss_set_option(const char* name, int value) {\n'
             '    for (unsigned i = 0; i < sizeof(rss_opts)/sizeof(rss_opts[0]); ++i) {\n'
             '        if (!strcmp(rss_opts[i].name, name)) {\n'
             '            rss_opts[i].staged = value;\n'
             '            rss_opts[i].dirty = 1;\n'
             '            return 1;\n'
             '        }\n'
             '    }\n'
             '    return 0;\n'
             '}\n\n'
             'extern void initSaver();\n'
             'extern void cleanUp();\n'
             'extern "C" void rss_restart(void) {\n'
             '    cleanUp();  // frees using the OLD option values\n'
             '    for (unsigned i = 0; i < sizeof(rss_opts)/sizeof(rss_opts[0]); ++i) {\n'
             '        if (rss_opts[i].dirty) {\n'
             '            *rss_opts[i].addr = rss_opts[i].staged;\n'
             '            rss_opts[i].dirty = 0;\n'
             '        }\n'
             '    }\n'
             '    initSaver();  // allocates using the NEW option values\n'
             '}\n')
    return body

def emit(saver, opts, out_path):
    lines = []
    if opts:
        width = max(len(n) for n, _, _, _ in opts)
        for name, _var, lo, hi in opts:
            rng = ''
            if lo is not None and hi is not None:
                # DEFAULTSn presets are sequential ints; show as 1..N.
                dlo = re.match(r'DEFAULTS(\d+)', lo or '')
                dhi = re.match(r'DEFAULTS(\d+)', hi or '')
                if dlo and dhi:
                    rng = '<%s..%s>  (presets)' % (dlo.group(1), dhi.group(1))
                else:
                    rng = '<%s..%s>' % (lo, hi)
            lines.append('  %-*s  %s' % (width, name, rng))
    table = '\n'.join(lines)
    body = (
        '/* AUTO-GENERATED by scripts/gen_help.py - do not edit.\n'
        ' * Per-saver option table for the SDL shell\'s --help. */\n\n'
        'extern const char* rss_saver_name;\n'
        'extern const char* rss_saver_options;\n\n'
        'const char* rss_saver_name = "%s";\n' % c_escape(saver)
    )
    if table:
        body += 'const char* rss_saver_options =\n'
        for ln in table.split('\n'):
            body += '    "%s\\n"\n' % c_escape(ln)
        body += '    ;\n'
    else:
        body += 'const char* rss_saver_options = "";\n'
    body += emit_runtime(opts)
    open(out_path, 'w').write(body)
    return len(opts)

def find_main_cpp(saver_dir):
    """The saver's main source. Usually <dir>.cpp, but solarwinds uses
    solarWinds.cpp (case differs), so match case-insensitively, preferring a
    file that actually defines handleCommandLine."""
    name = os.path.basename(saver_dir.rstrip('/'))
    cands = [f for f in os.listdir(saver_dir)
             if f.endswith('.cpp') and f != 'rss_help.cpp']
    for f in cands:
        if f[:-4].lower() == name.lower():
            return os.path.join(saver_dir, f)
    for f in cands:
        p = os.path.join(saver_dir, f)
        if 'handleCommandLine' in open(p, errors='replace').read():
            return p
    return None

def emit_json(saver, opts, defaults, preset_names, out_path):
    """Structured option metadata for the web shell's settings UI."""
    import json
    entries = []
    seen_vars = set()
    for name, var, lo, hi in opts:
        # CLI aliases bind two option names to one variable (flocks'
        # -colorfadespeed / -fadespeed); show only the first in the UI.
        if var in seen_vars:
            continue
        seen_vars.add(var)
        e = {'opt': name.lstrip('-')}
        dlo = re.match(r'DEFAULTS(\d+)', lo or '')
        dhi = re.match(r'DEFAULTS(\d+)', hi or '')
        if dlo and dhi:
            e['kind'] = 'preset'
            e['min'], e['max'] = int(dlo.group(1)), int(dhi.group(1))
            e['default'] = int(dlo.group(1))
            if preset_names:
                e['names'] = {str(k): v for k, v in sorted(preset_names.items())}
        else:
            try:
                e['min'], e['max'] = int(lo), int(hi)
            except (TypeError, ValueError):
                pass
            e['kind'] = 'bool' if (e.get('min') == 0 and e.get('max') == 1) else 'int'
            if var in defaults:
                d = defaults[var]
                # A few upstream defaults sit outside their own CLI range
                # (flux dWind=20, range 1..10); clamp so the slider can
                # represent "default" and Apply's no-change test holds.
                if 'min' in e:
                    d = min(max(d, e['min']), e['max'])
                e['default'] = int(d) if float(d).is_integer() else d
        entries.append(e)
    with open(out_path, 'w') as f:
        json.dump({'saver': saver, 'options': entries}, f, indent=1)
    return len(entries)

def process(cpp_path):
    saver = os.path.basename(os.path.dirname(cpp_path))
    opts = parse_options(cpp_path)
    defaults, preset_names = parse_defaults(cpp_path)
    out = os.path.join(os.path.dirname(cpp_path), 'rss_help.cpp')
    n = emit(saver, opts, out)
    jout = os.path.join(os.path.dirname(cpp_path), 'rss_options.json')
    emit_json(saver, opts, defaults, preset_names, jout)
    print('%-14s %2d options -> %s (+ rss_options.json)' % (saver, n, out))

def main(argv):
    if not argv:
        print(__doc__); return 1
    if argv[0] == '--all':
        root = os.path.join(os.path.dirname(__file__), '..', 'savers')
        for d in sorted(os.listdir(root)):
            sd = os.path.join(root, d)
            if not os.path.isdir(sd):
                continue
            cpp = find_main_cpp(sd)
            if cpp:
                process(cpp)
        return 0
    for p in argv:
        process(p)
    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
