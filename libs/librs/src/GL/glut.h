/*
 * GL/glut.h - rss-port mini-GLUT shim on SDL2.
 *
 * Implements just the slice of the (Free)GLUT API that the Implicit-surface
 * demo (savers/implicitdemo) needs: window creation, display + reshape
 * callbacks, the main loop, double-buffer swap, and redisplay posting.
 * Everything routes through SDL2 + the same ANGLE GLES2 / gl4es stack the
 * screensavers use, and under Emscripten the loop yields to the browser.
 *
 * This is deliberately tiny. It is NOT a general GLUT implementation: no
 * menus, keyboard/mouse callbacks, timers, fonts, idle func, or GLUT
 * geometry primitives. Add them here if a future demo needs them.
 *
 * LGPL 2.1 (same as the rest of librs).
 */

#ifndef RSS_MINIGLUT_H
#define RSS_MINIGLUT_H

#include <SDL.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gl4esinit.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* glutInitDisplayMode bit flags (values match GLUT) */
#define GLUT_RGB            0x0000
#define GLUT_RGBA           0x0000
#define GLUT_INDEX          0x0001
#define GLUT_SINGLE         0x0000
#define GLUT_DOUBLE         0x0002
#define GLUT_DEPTH          0x0010
#define GLUT_STENCIL        0x0020
#define GLUT_ALPHA          0x0008

/* ---- module-internal state (header-only; one translation unit includes it
 * with RSS_MINIGLUT_IMPL defined to emit the definitions) ---- */

typedef void (*rss_glut_display_cb)(void);
typedef void (*rss_glut_reshape_cb)(int, int);

#ifdef RSS_MINIGLUT_IMPL
SDL_Window*         rss_glut_window   = NULL;
SDL_GLContext       rss_glut_context  = NULL;
rss_glut_display_cb rss_glut_display  = NULL;
rss_glut_reshape_cb rss_glut_reshape  = NULL;
int                 rss_glut_running  = 1;
int                 rss_glut_w        = 400;
int                 rss_glut_h        = 400;
unsigned int        rss_glut_dmode    = GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH;
#else
extern SDL_Window*         rss_glut_window;
extern SDL_GLContext       rss_glut_context;
extern rss_glut_display_cb rss_glut_display;
extern rss_glut_reshape_cb rss_glut_reshape;
extern int                 rss_glut_running;
extern int                 rss_glut_w;
extern int                 rss_glut_h;
extern unsigned int        rss_glut_dmode;
#endif

#ifdef RSS_MINIGLUT_IMPL

static void* rss_glut_getproc(const char* name) {
    return (void*)SDL_GL_GetProcAddress(name);
}

static inline void glutInit(int* argcp, char** argv) {
    /* Honor --help/-h before opening a window: this is a standalone demo with
     * no saver options; just describe the live controls and exit. */
    if (argcp && argv) {
        for (int i = 1; i < *argcp; i++) {
            if (argv[i] && (!strcmp(argv[i], "--help") ||
                            !strcmp(argv[i], "-h"))) {
                printf("%s \xe2\x80\x94 Implicit-surface demo (SDL2 / OpenGL ES2)\n\n"
                       "No options. Live controls: Esc or Q to quit.\n",
                       argv[0] ? argv[0] : "implicitdemo");
                exit(0);
            }
        }
    }
#ifndef __EMSCRIPTEN__
    /* Use ANGLE's real GLES2 driver on desktop (see librs/rsSDLSaver.cpp). */
    SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1");
#ifdef __APPLE__
    /* ANGLE Metal backend: its desktop-GL default GPU-syncs on every
     * client-array draw (see librs/rsSDLSaver.cpp) */
    setenv("ANGLE_DEFAULT_PLATFORM", "metal", 0);
#endif
#endif
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
}

static inline void glutInitWindowSize(int w, int h) {
    rss_glut_w = w; rss_glut_h = h;
}

static inline void glutInitWindowPosition(int x, int y) {
    (void)x; (void)y;  /* SDL centers the window; position is advisory */
}

static inline void glutInitDisplayMode(unsigned int mode) {
    rss_glut_dmode = mode;
}

static inline int glutCreateWindow(const char* title) {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    if (rss_glut_dmode & GLUT_DOUBLE)
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    if (rss_glut_dmode & GLUT_DEPTH)
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    if (rss_glut_dmode & GLUT_STENCIL)
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    Uint32 rss_glut_wflags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
#ifndef __EMSCRIPTEN__
    /* Retina/HiDPI: match librs -- surface is pixel-scale, report pixels
     * via SDL_GL_GetDrawableSize (see rsSDLSaver.cpp) */
    rss_glut_wflags |= SDL_WINDOW_ALLOW_HIGHDPI;
#endif
    rss_glut_window = SDL_CreateWindow(title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        rss_glut_w, rss_glut_h,
        rss_glut_wflags);
    if (!rss_glut_window && (rss_glut_dmode & GLUT_DEPTH)) {
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);  /* retry shallower */
        rss_glut_window = SDL_CreateWindow(title,
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            rss_glut_w, rss_glut_h,
            rss_glut_wflags);
    }
    rss_glut_context = SDL_GL_CreateContext(rss_glut_window);
    SDL_GL_MakeCurrent(rss_glut_window, rss_glut_context);
    SDL_GL_SetSwapInterval(1);

    set_getprocaddress(rss_glut_getproc);
    initialize_gl4es();

    SDL_GL_GetDrawableSize(rss_glut_window, &rss_glut_w, &rss_glut_h);
    return 1;  /* window id (we only support one window) */
}

static inline void glutSetWindow(int win) { (void)win; }
static inline void glutDestroyWindow(int win) {
    (void)win;
    if (rss_glut_context) SDL_GL_DeleteContext(rss_glut_context);
    if (rss_glut_window)  SDL_DestroyWindow(rss_glut_window);
    SDL_Quit();
}

static inline void glutDisplayFunc(rss_glut_display_cb cb) { rss_glut_display = cb; }
static inline void glutReshapeFunc(rss_glut_reshape_cb cb) { rss_glut_reshape = cb; }

static inline void glutSwapBuffers(void) {
    /* gl4es batches geometry; flush before the SDL swap (see librs) */
    extern void gl4es_pre_swap(void);
    extern void gl4es_post_swap(void);
    gl4es_pre_swap();
    SDL_GL_SwapWindow(rss_glut_window);
    gl4es_post_swap();
}

/* GLUT redraws continuously; our loop already calls display every iteration,
 * so PostRedisplay is a no-op marker. */
static inline void glutPostRedisplay(void) { }

static inline void rss_glut_pump(void) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT)
            rss_glut_running = 0;
        else if (ev.type == SDL_KEYDOWN &&
                 (ev.key.keysym.sym == SDLK_ESCAPE || ev.key.keysym.sym == SDLK_q))
            rss_glut_running = 0;
        else if (ev.type == SDL_WINDOWEVENT &&
                 ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
            /* event data1/data2 are points; the GL surface is in pixels */
            SDL_GL_GetDrawableSize(rss_glut_window, &rss_glut_w, &rss_glut_h);
            if (rss_glut_reshape) rss_glut_reshape(rss_glut_w, rss_glut_h);
        }
    }
}

static inline void rss_glut_frame(void) {
    rss_glut_pump();
    if (rss_glut_display) rss_glut_display();
}

static inline void glutMainLoop(void) {
    if (rss_glut_reshape) rss_glut_reshape(rss_glut_w, rss_glut_h);
#ifdef __EMSCRIPTEN__
    /* ASYNCIFY build: a plain loop works because emscripten yields inside the
     * SDL swap, matching the screensaver shell. */
    while (rss_glut_running) rss_glut_frame();
#else
    while (rss_glut_running) rss_glut_frame();
#endif
}

#endif /* RSS_MINIGLUT_IMPL */

#ifdef __cplusplus
}
#endif

#endif /* RSS_MINIGLUT_H */
