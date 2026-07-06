/*
 * rsSDLSaver.cpp - SDL2 + GLES2(ANGLE) + gl4es shell for the Really Slick
 * Screensavers. Replaces rsWin32Saver / rsXScreenSaver.
 *
 * The saver provides (original rsXScreenSaver contract):
 *   void handleCommandLine(int argc, char* argv[]);
 *   void initSaver();
 *   void reshape(int width, int height);
 *   void idleProc();
 *   void cleanUp();
 *
 * Shell responsibilities: window + ES2 context creation, gl4es init,
 * event handling (exit on Esc/q or any input with --saver), suspend on
 * minimize, frame-rate limiting, browser main loop under Emscripten.
 *
 * Copyright (C) 2026 rss-port contributors. LGPL 2.1 (same as rslibs).
 */

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gl4esinit.h"  // initialize_gl4es(), set_getprocaddress()
#include <util/rsTimer.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// ---------------------------------------------------------------------------
// Framework state (see rsXScreenSaver/rsXScreenSaver.h)
// ---------------------------------------------------------------------------
int checkingPassword = 0;
int isSuspended = 0;
int doingPreview = 0;
int pfd_swap_exchange = 0;
int pfd_swap_copy = 0;
unsigned int dFrameRateLimit = 0;
int kStatistics = 0;

void* xdisplay = (void*)1;  // dummy non-NULL; only passed to glXSwapBuffers macro
void* xwindow = (void*)1;

// Saver hooks
extern void handleCommandLine(int argc, char* argv[]);
extern void initSaver();
extern void reshape(int width, int height);
extern void idleProc();
extern void cleanUp();

static SDL_Window* g_window = NULL;
static SDL_GLContext g_context = NULL;
static int g_running = 1;
static int g_saver_mode = 0;   // exit on any input, like a real screensaver
static int g_width = 1024, g_height = 640;
static int g_fullscreen = 1;

// gl4es frame hooks (exported in NOX11+NOEGL builds): pre_swap flushes any
// batched/pending geometry; post_swap rebinds gl4es's internal FBO if in use.
extern "C" void gl4es_pre_swap(void);
extern "C" void gl4es_post_swap(void);

extern "C" void rsSwapBuffers(void)
{
    gl4es_pre_swap();
    SDL_GL_SwapWindow(g_window);
    gl4es_post_swap();
}

static void* get_proc_address(const char* name)
{
    return (void*)SDL_GL_GetProcAddress(name);
}

// Parse shell-level args; saver-specific args go to handleCommandLine().
// Per-saver option table, generated into each saver's rss_help.cpp from its
// handleCommandLine() option list. Weak so savers that don't provide one (or
// the test saver) still link; NULL means "no saver-specific options".
#if defined(__GNUC__) || defined(__clang__)
__attribute__((weak)) const char* rss_saver_options = 0;
__attribute__((weak)) const char* rss_saver_name = 0;
#else
const char* rss_saver_options = 0;
const char* rss_saver_name = 0;
#endif

static void print_help(const char* argv0)
{
    const char* name = rss_saver_name ? rss_saver_name : argv0;
    printf("\n%s \xe2\x80\x94 Really Slick Screensaver (SDL2 / OpenGL ES2)\n\n", name);
    printf("Usage: %s [shell options] [saver options]\n\n", argv0);
    printf("Shell options:\n"
           "  --window WxH | -w WxH   run windowed at WxH (default 1024x640)\n"
           "  --fullscreen | -f       run fullscreen (default)\n"
           "  --saver                 screensaver mode: exit on any input\n"
           "  --fps-limit N           limit frame rate to N fps (0 = none)\n"
           "  --help | -h             show this help and exit\n\n");
    printf("Live controls (while running):\n"
           "  F1                      toggle on-screen stats / FPS\n"
           "  Esc or Q                quit\n\n");
    if (rss_saver_options && rss_saver_options[0]) {
        printf("Saver options (value in given range):\n%s\n", rss_saver_options);
    } else {
        printf("This saver takes no saver-specific options.\n\n");
    }
}

static void parse_shell_args(int argc, char* argv[])
{
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--window") || !strcmp(argv[i], "-w")) {
            g_fullscreen = 0;
            if (i + 1 < argc && sscanf(argv[i + 1], "%dx%d", &g_width, &g_height) == 2)
                i++;
        } else if (!strcmp(argv[i], "--fullscreen") || !strcmp(argv[i], "-f")) {
            g_fullscreen = 1;
        } else if (!strcmp(argv[i], "--saver")) {
            g_saver_mode = 1;
            g_fullscreen = 1;
        } else if (!strcmp(argv[i], "--fps-limit")) {
            if (i + 1 < argc) dFrameRateLimit = (unsigned int)atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
            print_help(argv[0]);
            exit(0);
        }
    }
#ifdef __EMSCRIPTEN__
    g_fullscreen = 0;  // browser canvas; size set below
#endif
}

// One-line discoverability banner printed at startup (native only).
static void print_startup_banner(const char* argv0)
{
#ifndef __EMSCRIPTEN__
    const char* name = rss_saver_name ? rss_saver_name : argv0;
    fprintf(stderr,
            "%s: F1 = stats, Esc/Q = quit. Run with --help for saver options.\n",
            name);
#else
    (void)argv0;
#endif
}

static void handle_events(void)
{
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
        case SDL_QUIT:
            g_running = 0;
            break;
        case SDL_KEYDOWN:
            if (g_saver_mode) { g_running = 0; break; }
            if (ev.key.keysym.sym == SDLK_ESCAPE || ev.key.keysym.sym == SDLK_q)
                g_running = 0;
            else if (ev.key.keysym.sym == SDLK_F1)
                kStatistics = !kStatistics;
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEMOTION:
            if (g_saver_mode && ev.type == SDL_MOUSEBUTTONDOWN) g_running = 0;
            break;
        case SDL_WINDOWEVENT:
            if (ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                g_width = ev.window.data1;
                g_height = ev.window.data2;
                reshape(g_width, g_height);
            } else if (ev.window.event == SDL_WINDOWEVENT_MINIMIZED ||
                       ev.window.event == SDL_WINDOWEVENT_HIDDEN) {
                isSuspended = 1;
            } else if (ev.window.event == SDL_WINDOWEVENT_RESTORED ||
                       ev.window.event == SDL_WINDOWEVENT_SHOWN ||
                       ev.window.event == SDL_WINDOWEVENT_EXPOSED) {
                isSuspended = 0;
            }
            break;
        }
    }
}

static rsTimer g_frame_timer;

static void main_frame(void)
{
    handle_events();
    idleProc();  // saver draws and swaps (glXSwapBuffers -> rsSwapBuffers)
#ifndef __EMSCRIPTEN__
    if (dFrameRateLimit > 0)
        g_frame_timer.wait(1.0 / (double)dFrameRateLimit);
#endif
}

int main(int argc, char* argv[])
{
    parse_shell_args(argc, argv);
    print_startup_banner(argv[0]);

#ifndef __EMSCRIPTEN__
    // Force SDL to load a real OpenGL ES driver. On desktop (Windows/Linux/mac)
    // this makes SDL use the vendored ANGLE libGLESv2/libEGL — i.e. a genuine
    // GLES2 context — instead of falling back to the platform's desktop-GL
    // path, which is what gl4es expects underneath. Must be set before
    // SDL_Init (the video subsystem reads it during initialization). Under
    // Emscripten the driver is always WebGL, so this hint does not apply.
    SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1");
#endif

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    if (g_fullscreen) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

    g_window = SDL_CreateWindow("rss-port",
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                g_width, g_height, flags);
    if (!g_window) {
        // Depth 24 can fail on some GLES drivers; retry with 16.
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
        g_window = SDL_CreateWindow("rss-port",
                                    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                    g_width, g_height, flags);
    }
    if (!g_window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return 1;
    }

    g_context = SDL_GL_CreateContext(g_window);
    if (!g_context) {
        fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_GL_MakeCurrent(g_window, g_context);
    SDL_GL_SetSwapInterval(1);  // vsync

    // gl4es: resolve GLES2 entry points through SDL, then init
    set_getprocaddress(get_proc_address);
    initialize_gl4es();

    SDL_GL_GetDrawableSize(g_window, &g_width, &g_height);

    if (g_saver_mode) SDL_ShowCursor(SDL_DISABLE);

    handleCommandLine(argc, argv);
    reshape(g_width, g_height);
    initSaver();

#ifdef __EMSCRIPTEN__
    // ASYNCIFY build: plain loop works because emscripten can yield inside
    // SDL_GL_SwapWindow. (Matches the sgi-demos approach.)
    while (g_running)
        main_frame();
#else
    while (g_running)
        main_frame();
#endif

    cleanUp();

    SDL_GL_DeleteContext(g_context);
    SDL_DestroyWindow(g_window);
    SDL_Quit();
    return 0;
}
