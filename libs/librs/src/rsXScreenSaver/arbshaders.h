/*
 * arbshaders.h - rss-port shim for the Really Slick savers' ARB shader-object
 * extension loader (hyperspace/extensions.{h,cpp}, microcosm/extensions.{h,cpp}).
 *
 * Upstream, these savers load GL_ARB_shader_objects / GL_ARB_multitexture /
 * GL_ARB_texture_cube_map entry points at runtime via wglGetProcAddress and
 * gate shader use on their presence (Windows only; the Linux/xscreensaver
 * builds never enabled shaders).
 *
 * gl4es exports all of these ARB entry points natively as real linkable
 * symbols (glCreateShaderObjectARB, glUseProgramObjectARB, ...), and its
 * built-in GLSL converter (shaderconv) rewrites the desktop GLSL 1.10 the
 * savers supply into the GLSL ES 1.00 that the GLES2/WebGL backend accepts.
 *
 * So on the rss-port path we bypass the runtime loader entirely: this header
 * makes the savers' extern function-pointer names resolve directly to gl4es's
 * functions, and provides an initExtensions() that simply reports success.
 *
 * Include this INSTEAD of the saver's own extensions.h on the SDL/gl4es path.
 */

#ifndef RSS_ARBSHADERS_H
#define RSS_ARBSHADERS_H

#include <GL/gl.h>
#include <GL/glext.h>

// gl4es provides all of these ARB entry points as real functions. On Linux
// they keep their ARB names; on Emscripten/macOS gl4es mangles them (via
// gl_mangle.h, included by <GL/gl.h>), so a call to glCreateShaderObjectARB
// transparently resolves to gl4es_glCreateShaderObjectARB. We must NOT
// redefine the names here (a self-referential #define would shadow the
// mangle and leave the symbol undefined). We only need GL/gl.h's prototypes,
// pulled in above, plus initExtensions() reporting success.
//
// Exception: on the platforms where gl4es mangles, it does NOT emit the
// ARB-suffixed aliases at all (only the core GL 2.0 names exist, as
// gl4es_glCreateShader etc.). The ARB shader-object API is functionally
// identical to core GL 2.0, so map the ARB names onto the core names there;
// gl4es's gl_mangle.h then rewrites the core name to its gl4es_ symbol.
#if defined(__EMSCRIPTEN__) || defined(__APPLE__)
  typedef GLuint GLhandleARB;
  #undef glCreateShaderObjectARB
  #undef glShaderSourceARB
  #undef glCompileShaderARB
  #undef glCreateProgramObjectARB
  #undef glAttachObjectARB
  #undef glLinkProgramARB
  #undef glUseProgramObjectARB
  #undef glGetUniformLocationARB
  #undef glUniform1iARB
  #undef glUniform3fARB
  #undef glActiveTextureARB
  #define glCreateShaderObjectARB(type)        glCreateShader(type)
  #define glShaderSourceARB(s,c,str,len)       glShaderSource(s,c,str,len)
  #define glCompileShaderARB(s)                glCompileShader(s)
  #define glCreateProgramObjectARB()           glCreateProgram()
  #define glAttachObjectARB(p,s)               glAttachShader(p,s)
  #define glLinkProgramARB(p)                  glLinkProgram(p)
  #define glUseProgramObjectARB(p)             glUseProgram(p)
  #define glGetUniformLocationARB(p,n)         glGetUniformLocation(p,n)
  #define glUniform1iARB(l,v)                  glUniform1i(l,v)
  #define glUniform3fARB(l,a,b,c)              glUniform3f(l,a,b,c)
  #define glActiveTextureARB(t)                glActiveTexture(t)
#endif

// gl4es exposes these as prototypes (we compile with GL_GLEXT_PROTOTYPES),
// so no function-pointer variables are needed. initExtensions() just reports
// that shader support is available.
static inline int initExtensions() { return 1; }

#endif // RSS_ARBSHADERS_H
