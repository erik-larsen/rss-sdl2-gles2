/* Force-included (-include) into every native MinGW compile; see the win
 * block in makefiles/platform.mk.
 *
 * In the saver sources WIN32 fences the original Win32 screensaver shell
 * (rsWin32Saver, registry, dialogs, wgl extension loading); the port runs
 * the same SDL path as Linux/macOS. MinGW predefines WIN32, and its
 * <minwindef.h> defines it again whenever <windows.h> is included, so a
 * command-line -UWIN32 does not stick. Including <windows.h> here first and
 * then undefining WIN32 does: the include guards keep later <windows.h>
 * includes from restoring it. Code that really is Windows-specific (rsTimer,
 * aligned alloc) keys on _WIN32 instead.
 *
 * NOMINMAX stops <windows.h> from defining min/max macros over the savers'
 * own helpers; WIN32_LEAN_AND_MEAN matches what gl4es's GL/gl.h asks for.
 */
#ifndef RSS_MINGW_H
#define RSS_MINGW_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#ifndef NOMINMAX
#define NOMINMAX 1
#endif
#include <windows.h>
#undef WIN32

#endif
