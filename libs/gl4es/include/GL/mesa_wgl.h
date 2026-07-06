/* Stub for Mesa's mesa_wgl.h, which gl4es's GL/gl.h #includes on MinGW but
 * gl4es does not ship. The wgl* API is already declared by <windows.h>
 * (included just above in gl.h), and nothing in this project calls wgl
 * directly -- contexts come from SDL2/ANGLE. */
#ifndef MESA_WGL_H
#define MESA_WGL_H
#endif
