/*
 * rsXScreenSaver.h (SDL2 compatibility version)
 *
 * Part of the rss-port librs SDL2 screensaver shell. This header keeps the
 * original savers' `#ifdef RS_XSCREENSAVER` code paths compiling unchanged,
 * while the windowing/context work is actually done by SDL2 + ANGLE GLES2
 * with gl4es translating the savers' OpenGL 1.x calls.
 *
 * Original rsXScreenSaver library: Copyright (C) 2006 Terence M. Welsh,
 * LGPL 2.1. This compatibility layer is distributed under the same license.
 */

#ifndef RSXSCREENSAVER_H
#define RSXSCREENSAVER_H

#include <GL/gl.h>

#include <util/rsTimer.h>
#include <util/arguments.h>

// ---------------------------------------------------------------------------
// Framework state the savers read
// ---------------------------------------------------------------------------
extern int checkingPassword;   // always 0 under SDL shell
extern int isSuspended;        // 1 when window minimized/hidden
extern int doingPreview;       // always 0 under SDL shell

extern int pfd_swap_exchange;  // legacy win32 pixel-format info; 0
extern int pfd_swap_copy;      // legacy win32 pixel-format info; 0

extern unsigned int dFrameRateLimit;  // 0 = no limit
extern int kStatistics;               // toggled with F1

// ---------------------------------------------------------------------------
// X11 impersonation: savers call glXSwapBuffers(xdisplay, xwindow) in draw().
// Route that to SDL_GL_SwapWindow without the saver knowing.
// ---------------------------------------------------------------------------
typedef void* rsDisplay;
typedef void* rsWindow;
#define Display rsDisplayCompat
#define Window  rsWindowCompat
typedef void* rsDisplayCompat;
typedef void* rsWindowCompat;

extern Display* xdisplay;  // dummy, non-NULL
extern Window xwindow;     // dummy

// Win32 types used by some savers' settings globals
#ifndef BOOL
typedef int BOOL;
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifdef __cplusplus
extern "C" {
#endif
void rsSwapBuffers(void);
#ifdef __cplusplus
}
#endif

#define glXSwapBuffers(dpy, win) rsSwapBuffers()

// ---------------------------------------------------------------------------
// Hooks each saver must provide (same contract as original rsXScreenSaver):
//   void handleCommandLine(int argc, char* argv[]);
//   void initSaver();
//   void reshape(int width, int height);
//   void idleProc();
//   void cleanUp();
// ---------------------------------------------------------------------------

#endif // RSXSCREENSAVER_H
