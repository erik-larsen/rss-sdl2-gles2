# Porting notes

## Decisions log (Milestone 0)

1. **Savers compile with -DRS_XSCREENSAVER, unmodified.** librs provides a
   compat header at `rsXScreenSaver/rsXScreenSaver.h` mirroring the original
   framework's externs and hook contract (handleCommandLine / initSaver /
   reshape / idleProc / cleanUp), and `#define glXSwapBuffers(d,w) rsSwapBuffers()`.
2. **gl4es config:** STATICLIB + NOX11 + NOEGL + NO_LOADER + NO_GBM +
   NO_INIT_CONSTRUCTOR + DEFAULT_ES=2. NO_INIT_CONSTRUCTOR is required:
   without it gl4es self-initializes in a static constructor before main(),
   crashing (no GL context yet). The shell calls set_getprocaddress(SDL_GL_GetProcAddress)
   then initialize_gl4es() after context creation.
3. **GL_GLEXT_PROTOTYPES is defined globally** so gl4es's headers expose
   GL 1.5/2.0 entry points (glBindBuffer etc.) as direct prototypes —
   needed by Implicit/impSurface.cpp's non-WIN32 path.
4. **glues headers patched** (`source/*.h`): added a default platform branch
   including gl4es's <GL/gl.h> (upstream supported only SDL-GLES1/QNX/Win32
   and #error'd otherwise). This is the only vendored-library modification.
5. **arguments.cpp reimplemented.** rslibs ships rsUtility/arguments.h with
   no implementation anywhere in the repo; a clean-room implementation
   matching the documented semantics lives in libs/rsmath/src/util/arguments.cpp.
6. **Linux link note:** the vendored ANGLE libGLESv2.so references
   libXNVCtrl.so.0 (package `libxnvctrl0` on Debian/Ubuntu). Needed at
   link and run time on Linux.

## Per-saver adaptation procedure (established with flux)

1. `mkdir savers/<name>` ; copy upstream .cpp/.h verbatim.
2. 2-line Makefile: `APPNAME = <name>` + `include ../../makefiles/make_saver.mk`.
3. `make native` — fix only what genuinely fails; prefer fixing librs/shims
   over editing saver source.
4. Run windowed under the shell; screenshot-verify against upstream visuals.

## Decisions log (Milestone 1)

7. **gl4es swap hooks are mandatory.** gl4es batches immediate-mode geometry;
   calling SDL_GL_SwapWindow directly leaves the batch unflushed, so frames
   appear empty or partial (euphoria rendered fully black). librs's
   rsSwapBuffers now calls gl4es_pre_swap() / gl4es_post_swap() (exported in
   NOX11+NOEGL builds) around the SDL swap. This fixed euphoria and improves
   frame completeness for every saver.
8. **Windows-only savers get the flux-style dual path.** cyclone, fieldlines,
   euphoria had no RS_XSCREENSAVER support upstream; they received the same
   minimal #ifdef structure flux/solarwinds already had: fenced win32
   includes/registry/dialog code, an RS reshape() carrying the projection
   setup, RS initSaver()/cleanUp() variants, handleCommandLine() mapping the
   registry options to original-style flags, readyToDraw set at end of
   initSaver (upstream set it in the win32 WndProc).
9. **DEFAULTS presets:** savers with preset systems define DEFAULTS1..N
   locally under RS_XSCREENSAVER (flux pattern); upstream took these IDs from
   win32 resource.h. euphoria has 9 presets (-default 1..9).
10. **rsText leaves its font texture bound** after construction; harmless so
   far but worth remembering when debugging texture state.

## Decisions log (Milestone 2)

11. **flocks, plasma: zero source changes.** Both had upstream RS_XSCREENSAVER
   paths; plasma's per-frame glTexSubImage2D dynamic-texture upload works fine
   through gl4es.
12. **glues gluBuild2DMipmaps now uses gl4es hardware mipmap generation**
   (GLUES_USE_HW_MIPMAP, set in libs/glues/Makefile). The portable CPU mipmap
   builder uploaded chains that gl4es treated as incomplete for power-of-two
   GL_RGB/RGBA, so any *_MIPMAP_* min filter sampled the texture as black
   (helios spheremap, and would have hit lattice's chrome/brass textures too).
   For POT input we now enable GL_GENERATE_MIPMAP and upload only level 0;
   NPOT still uses the CPU path. Verified across LUMINANCE/RGB/RGBA/ALPHA.
13. **Implicit/impSurface VBO path disabled under gl4es.** The buffer-offset
   glDrawElements path segfaulted; switched to client-side vertex arrays
   (mUseVBOs=false on non-WIN32). Also unbind GL_ARRAY_BUFFER/ELEMENT_ARRAY
   after drawing — a stale binding blacks out gl4es immediate-mode geometry.
14. **helios:** Windows-only upstream; given the standard dual-path treatment.
   Two swap sites in draw() (one was already commented-out dead code). Uses
   gluSphere via Implicit metaball surfaces + GL_SPHERE_MAP texgen, all working.
15. **lattice:** multi-file (camera.cpp). Projection matrix is custom (not
   gluPerspective) and feeds the camera frustum object, so it was factored into
   a setProjection() that reshape() re-invokes on resize. 9 texture presets
   (regular/chainmail/brass/chrome/etc.), all rendering.

## Decisions log (Milestone 3 - the GLSL shader savers)

16. **gl4es exports the ARB shader-object API natively** (glCreateShaderObjectARB,
   glUseProgramObjectARB, glUniform3fARB, ...), so the savers' runtime extension
   loaders (extensions.cpp) are unnecessary on the gl4es path. librs ships
   rsXScreenSaver/arbshaders.h: it provides initExtensions()->1 and lets the
   saver source call the ARB functions directly (they link to gl4es). The
   savers' own extensions.cpp are renamed extensions.cpp.win32 (kept for a
   future native-desktop-GL Windows build, excluded from the gl4es build glob).
17. **gl4es has a built-in GLSL converter (shaderconv)** that rewrites desktop
   GLSL 1.10 (gl_ModelViewMatrix, ftransform, gl_FrontColor, gl_Fog,
   gl_LightSource, gl_MultiTexCoord0, gl_TexCoord, texture2D/textureCube) into
   GLSL ES 1.00 automatically. No manual shader rewrite needed for the common
   built-ins. This did the heavy lifting for both savers.
18. **Two small shader-source edits were still required:**
    a. Remove the savers' manual `varying vec4 gl_TexCoord[1];` redeclarations
       (4 in hyperspace, 2 in microcosm). These were a 2005-era driver
       workaround; they collide with shaderconv's own gl_TexCoord emission
       (it rewrites both to `_gl4es_TexCoord`, producing a redeclaration error).
    b. microcosm: convert sampler1D/texture1D -> sampler2D/texture2D with
       v=0.5. GLES2/WebGL have no 1D textures and shaderconv does not rewrite
       user 1D samplers (only its own FPE ones). gl4es's glTexImage1D already
       stores GL_TEXTURE_1D as an Nx1 GL_TEXTURE_2D, so sampling the same unit
       as 2D at the row center works. The binding-side code is unchanged.
19. Both savers default dShaders=1 and now call initExtensions() on the RS path
   (previously WIN32-only); it always succeeds under gl4es. The savers retain
   their non-shader fallback code paths for dShaders=0 if ever needed.

## Decisions log (Milestone 4 - the stragglers)

20. **implicitDemo: mini-GLUT on SDL.** implicitDemo is a standalone demo of
    the Implicit isosurface library (morphing sphere/torus/knot/...), written
    against FreeGLUT, not the saver framework. It uses only 12 GLUT calls, each
    once: init/window/display+reshape callbacks/mainloop/swap/redisplay. Rather
    than depend on FreeGLUT, librs ships a header-only GL/glut.h implementing
    exactly that slice on SDL2 + gl4es (define RSS_MINIGLUT_IMPL in the one TU
    that has main()). Reusable for any future GLUT-based demo; deliberately not
    a full GLUT (no menus/input/timers/fonts/primitives).
21. **skyrocket: optional OpenAL, muted-capable.** Largest saver (fireworks,
    7 source files), Windows-only upstream, with positional 3D audio via OpenAL.
    soundEngine.{h,cpp} were made portable: HWND->void*, no <windows.h>, and the
    entire OpenAL body guarded by RSS_USE_OPENAL. Without that define the engine
    is a silent stub with the same interface (insertSoundNode still does its
    bookkeeping), so skyrocket builds and runs muted. With it, native links
    OpenAL (auto-detected via pkg-config) and Emscripten uses its built-in
    OpenAL (-lopenal). All saver-side sound calls were already if(soundengine)
    guarded. OpenAL's no-audio-device fallback is graceful (device==NULL check),
    so it is safe to enable even on headless/soundless systems.
22. **skyrocket misc portability:** guarded <windows.h> in flare.h/smoke.h/
    shockwave.cpp; std::max/min in flare.cpp (max was a windows.h macro);
    MK_LBUTTON/RBUTTON/MBUTTON defined for non-WIN32 (the mouse-camera
    interactive mode is win32 WndProc-driven; mouseButtons stays 0 on the RS
    path so those branches never execute, but the symbols must exist).

## All 13 RSS savers + testsaver now build and render natively (Linux/gl4es/ANGLE).
Remaining: Emscripten browser builds (written but unverified - no emsdk in the
dev sandbox), web gallery, CI, and optional OS screensaver packaging.

## Decisions log (Milestone 5 - Emscripten / browser)

23. **Toolchain:** the official emsdk downloads its LLVM/node from
    storage.googleapis.com. In CI (real GitHub Actions) mymindstorm/setup-emsdk
    works directly. The build only needs emcc/em++/emar on PATH plus host cmake
    (for gl4es). gl4es source already has __EMSCRIPTEN__ handling; emcmake's
    toolchain defines EMSCRIPTEN so no CMakeLists change was needed.
24. **CRITICAL (also affects macOS native): gl4es mangles all GL/GLU symbols on
    __EMSCRIPTEN__ AND __APPLE__** (gl_mangle.h / glu_mangle.h, e.g.
    gluPerspective -> mgluPerspective) to avoid clashing with the platform GL.
    glues' GLU *definitions* were not being mangled, so they didn't match the
    savers' (mangled) calls -> undefined symbols at link. Fixed by including
    GL/glu_mangle.h in every glues header on those two platforms, so glues
    exports mglu* there and plain glu* on Linux. Native Linux unaffected.
25. **gl4es native vs web archive collision:** gl4es' CMake always writes
    lib/libGL.a regardless of build dir, so the web build clobbered the native
    lib. Now each build uses its own CMake build dir and the result is copied to
    distinct paths: libnative/libGL.a (GL4ES_LIB) and lib-web/libGL.a
    (EM_GL4ES_LIB).
26. **Per-target compile flags:** added NATIVE_LIB_CFLAGS / EM_LIB_CFLAGS to
    make_lib.mk so host SDL (sdl2-config) is used natively while the web build
    uses emscripten's SDL port (-sUSE_SDL=2). Same idea already present for
    savers in make_saver.mk.
27. **Heap + GL temp buffer (link flags, EM_MEMORY):**
    -sALLOW_MEMORY_GROWTH=1 with 64MB initial (plasma's plasma texture needs
    ~40MB), and -sGL_MAX_TEMP_BUFFER_SIZE=33554432. The latter fixes a crash in
    hyperspace: gl4es batches immediate-mode geometry into large client-side
    vertex arrays, and emscripten's FULL_ES2 path copies these through a temp
    VBO whose 2MB default was too small, indexing past its ring-buffer array
    (TypeError reading 'undefined' in getTempVertexBuffer). 32MB covers it.
28. **ASYNCIFY** lets the savers' unmodified native while(running) loop yield to
    the browser inside SDL_GL_SwapWindow (set in EM_ASYNCIFY).
29. **Custom shell** (libs/librs/shell.html, --shell-file): fullscreen black
    canvas, minimal load UI, Esc/Q to exit, and a preRun that sets LIBGL_USEVBO/
    LIBGL_BATCH env for gl4es. Replaces emscripten's default branded template.
30. **Verification:** built and rendered all 14 targets in headless Chromium +
    SwiftShader (software WebGL) over HTTP via Puppeteer; screenshots confirm
    correct output for every saver including the shader (hyperspace/microcosm),
    texture (lattice/helios), and audio (skyrocket) savers. Real-GPU WebGL on
    end-user machines may differ slightly from SwiftShader but correctness is
    established. scripts/deploy-web.sh assembles web/ (gallery + all savers);
    scripts/gallery.html is the index template; scripts/serve_web.py serves it.
31. **CI:** .github/workflows/build.yml runs a native matrix (ubuntu/macos/
    windows-msys2), a web build job (setup-emsdk), and deploys the gallery to
    GitHub Pages on main.

## All 13 RSS savers + testsaver build and render on Linux, and as WebAssembly.
macOS/Windows native builds follow the same makefiles (CI covers them) but were
not run in the dev sandbox. Remaining optional work: OS screensaver packaging
(.scr / .saver / xscreensaver hack mode), per-GPU testing, and audio tuning.

## Decisions log (post-M5: ANGLE driver selection)

32. **SDL_HINT_OPENGL_ES_DRIVER = "1" set before SDL_Init** (in librs's
    rsSDLSaver.cpp and the mini-GLUT glutInit), guarded out under Emscripten.
    On desktop this tells SDL to load a real OpenGL ES driver, so it uses the
    vendored ANGLE libGLESv2/libEGL (a genuine GLES2 context) instead of
    possibly falling back to the platform's desktop-GL path — which is the
    context gl4es expects underneath. Belt-and-suspenders alongside the
    SDL_GL_CONTEXT_PROFILE_ES attribute + ANGLE on the rpath; particularly
    relevant on machines where a system desktop-GL could otherwise be selected.
    Under Emscripten the driver is always WebGL, so the hint is omitted there.

## Decisions log (post-M5: web controls HUD)

33. **Controls-legend HUD (web builds).** libs/librs/shell.html now overlays a
    small auto-fading panel (top-right) listing the live controls, which are
    uniform across all savers: F1 toggles stats, Esc exits fullscreen, H shows/
    hides the legend, and a clickable "Fullscreen" row requests browser
    fullscreen on the canvas. The panel appears on load, then ~4s later
    collapses to a faint "?" button so the saver is unobstructed; H or the
    button reopens it. The H key is handled in the page (capture phase) so it
    never reaches the wasm canvas. The panel title is derived from the page's
    own filename in the URL (e.g. /flux/flux.html -> "Flux"), available
    immediately without waiting for emscripten's async script tag.

    Note: the RSS savers read their per-saver settings once at initSaver() and
    have no live keyboard/mouse handling of their own (skyrocket's mouse-camera
    code is Windows-WndProc-only and inert on this path), so the live control
    set really is just the shell's. Exposing the rich per-saver *launch* options
    (flux: -particles/-blur/..., etc.) as a live web settings panel would
    require teaching each handleCommandLine to read URL query params and
    reloading; deferred as a future pass.

    Native builds surface controls differently (planned): print the control
    legend + per-saver option list to the console at startup / on --help.

## Decisions log (post-M5: native console help)

34. **Native --help + startup banner.** The SDL shell (rsSDLSaver.cpp) now
    prints a full help screen on --help/-h: the saver name, shell options, the
    live-control legend (F1 stats, Esc/Q quit), and the saver's own option table
    with valid ranges. At startup it also prints a one-line stderr banner
    ("<saver>: F1 = stats, Esc/Q = quit. Run with --help for saver options.")
    for discoverability. Both are #ifndef __EMSCRIPTEN__ (web uses the HUD).

35. **Per-saver option tables are generated, not hand-written.** Each saver's
    options live in its handleCommandLine() as getArgumentsValue(..., "-name",
    var, min, max) calls. scripts/gen_help.py parses those and emits
    savers/<name>/rss_help.cpp defining weak-linked `rss_saver_name` and
    `rss_saver_options` (the formatted "-name <min..max>" table; DEFAULTSn
    preset ranges shown as "1..N (presets)"). The shell declares these symbols
    __attribute__((weak)) so savers without a table (testsaver) still link and
    print "takes no saver-specific options". make_saver.mk globs *.cpp so
    rss_help.cpp is compiled automatically. Regenerate with `make help-gen`
    after changing a saver's options. Handles solarwinds' solarWinds.cpp
    case-mismatch via find_main_cpp().

36. **implicitDemo --help:** it has its own main() via mini-GLUT (so the shell's
    rsSDLSaver.o, and thus the shell --help, is never linked). glutInit() now
    short-circuits --help/-h with a one-line message before opening a window.

37. **Build fix (found here):** make_saver.mk object rules now depend on
    $(LIBRS_HDRS) (librs/src/**/*.h), so edits to the shell shim / mini-GLUT /
    ARB-shader / help headers correctly trigger saver rebuilds. Previously only
    local saver *.h were dependencies.

## Decisions log (post-M5: gl4es build fixes from ogl-win-savers)

38. **Adopted the ogl-win-savers gl4es Makefile verbatim** (user-provided; the
    two projects vendor the identical gl4es CMakeLists). Changes vs the old
    rss-port wrapper:
    a. GL4ES_QUIET: -Wno-macro-redefined -Wno-dangling-else
       -Wno-logical-not-parentheses -Wno-parentheses via CMAKE_C_FLAGS. These
       are the vendored-code diagnostics newer clang trips on; on the user's
       machine they surfaced as the 2 build-summary errors. Benign categories
       only, so our own code stays fully warned.
    b. RelWithDebInfo instead of Release.
    c. Output naming: lib/libGL-native.a and lib/libGL-web.a (single lib/ dir,
       target-suffixed) replacing libnative/ + lib-web/. platform.mk updated.
    d. **Web config keeps the gl4es loader and auto-init constructor ENABLED**
       (no NO_LOADER / NO_INIT_CONSTRUCTOR for the emscripten build). Under
       FULL_ES2 the GLES entry points are statically linked and
       SDL_GL_GetProcAddress can return null for them on newer emsdk, so a
       NO_LOADER web build has no way to reach GL and renders black
       (ogl-win-savers finding; our CI uses emsdk 3.1.64 where this applies).
       The host's explicit initialize_gl4es() call remains and is idempotent.
       Native config still disables both (the constructor firing before a
       context exists is the M0 segfault).
    Verified after adoption: full native rebuild green (all 14), flux renders
    natively; gl4es web rebuilt with the new config and flux, hyperspace, and
    skyrocket re-verified rendering in headless Chromium with zero errors.

## Decisions log (post-M5: first real macOS-native build)

39. **macOS arm64 native build verified** (previously only covered on paper by
    CI makefiles; the dev sandbox was Linux). Two fixes were needed:
    a. **gl4es AliasDecl on darwin:** clang's `__GNUC__` branch of AliasDecl
       uses `__attribute__((alias))`, which darwin's ld does not support
       (upstream gl4es has the same gap — AliasExport is __APPLE__-guarded,
       AliasDecl is not). The only two uses (gl4es_glEnable/DisableClientStatei
       in directstate.c) are now explicit forwarding wrappers under
       `#ifdef __APPLE__`; real definitions are required because gl_lookup.c
       references both symbols. All 14 targets then build clean
       (bin-mac-arm64); flux, helios, and skyrocket smoke-run over
       ANGLE/Metal ("Hardware vendor is Google Inc. (Apple)").
    b. **rgbhsl case-collision fix:** git tracked both
       libs/rsmath/src/Rgbhsl/Rgbhsl.h and a lowercase shim
       libs/rsmath/src/rgbhsl/rgbhsl.h (for helios's `#include
       <rgbhsl/rgbhsl.h>`). On case-insensitive filesystems (macOS/Windows —
       including the CI runners) the two paths are one file, so checkout
       clobbers one with the other and the tree is permanently dirty. The shim
       moved to include/shim/rgbhsl/rgbhsl.h (SHIM_INC was already on the
       saver include path) and the colliding lowercase path was removed from
       git. On macOS clang instead resolves the include case-insensitively to
       Rgbhsl/Rgbhsl.h directly and emits one benign
       -Wnonportable-include-path warning in helios; Linux/CI uses the shim,
       warning-free. The shim's include guard (RGBHSL_SHIM_H) protects against
       self-inclusion if include-path order ever puts SHIM_INC first.
