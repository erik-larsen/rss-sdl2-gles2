# Platform specifics (adapted from sgi-demos/makefiles/platform.mk)
OS:=$(shell uname -s)
HW:=$(shell uname -m)
ifeq ($(OS),Darwin)
	OS = mac
	DYL_EXT = dylib
	CONSOLE_FLAGS =
else ifeq ($(OS),Linux)
	OS = linux
	DYL_EXT = so
	CONSOLE_FLAGS =
else ifneq ($(findstring MINGW64_NT,$(OS)),)
	OS = win
	DYL_EXT = dll
	CONSOLE_FLAGS = -mconsole
endif

# Source and output dirs (relative to a saver/lib dir, two levels deep)
SRC_DIR = .
BIN_DIR = ./bin-$(OS)-$(HW)
WEB_DIR = ./web
LIBS_DIR = ../../libs
INCS_DIR = ../../include

# SDL libs and include
SDL_LIBS = `sdl2-config --libs`
EM_SDL_LIBS = -s USE_SDL=2 -s FULL_ES2=1
SDL_INC = `sdl2-config --cflags`

# GLES (ANGLE) libs and include
GLES_LIBS_PATH = $(LIBS_DIR)/libgles/lib-$(OS)
GLES_INC = -I$(LIBS_DIR)/libgles/include
GLES_LIBS = -L$(GLES_LIBS_PATH) -l GLESv2 -l EGL
GLES_DYLIBS = libGLESv2.$(DYL_EXT) libEGL.$(DYL_EXT)
ifeq ($(OS),mac)
	GLES_LINK = -Wl,-rpath,$(GLES_LIBS_PATH)
define GLES_INSTALL
	for dylib in $(GLES_DYLIBS); \
		do install_name_tool -change ./$$dylib @rpath/$$dylib $@; \
	done;
endef
else ifeq ($(OS),linux)
	GLES_LINK = -Wl,-rpath,$(GLES_LIBS_PATH)
else ifeq ($(OS),win)
define GLES_INSTALL
	for dylib in $(GLES_DYLIBS); \
		do cp $(GLES_LIBS_PATH)/$$dylib $(BIN_DIR)/$$dylib; \
	done;
endef
endif

# gl4es: OpenGL 1.x/2.x -> GLES2 translation (static lib)
GL4ES_DIR = $(LIBS_DIR)/gl4es
GL4ES_LIB = $(GL4ES_DIR)/lib/libGL-native.a
EM_GL4ES_LIB = $(GL4ES_DIR)/lib/libGL-web.a
# gl4es GL headers must come first so <GL/gl.h> resolves to gl4es;
# GL_GLEXT_PROTOTYPES exposes GL 1.5+/2.0 entry points as direct prototypes
GL4ES_INC = -I$(GL4ES_DIR)/include -DGL_GLEXT_PROTOTYPES

# glues: GLU on top of gl4es (static lib)
GLUES_DIR = $(LIBS_DIR)/glues
GLUES_LIB = $(GLUES_DIR)/$(BIN_DIR)/libglues.a
EM_GLUES_LIB = $(GLUES_DIR)/$(WEB_DIR)/libglues.a
GLUES_INC = -I$(GLUES_DIR)/include

# rsmath: rsMath + Rgbhsl + rsText + Implicit from rslibs (static lib)
RSMATH_DIR = $(LIBS_DIR)/rsmath
RSMATH_LIB = $(RSMATH_DIR)/$(BIN_DIR)/librsmath.a
EM_RSMATH_LIB = $(RSMATH_DIR)/$(WEB_DIR)/librsmath.a
RSMATH_INC = -I$(RSMATH_DIR)/src

# librs: SDL2 screensaver shell (replaces rsWin32Saver / rsXScreenSaver)
LIBRS_DIR = $(LIBS_DIR)/librs
LIBRS_LIB = $(LIBRS_DIR)/$(BIN_DIR)/librs.a
EM_LIBRS_LIB = $(LIBRS_DIR)/$(WEB_DIR)/librs.a
LIBRS_INC = -I$(LIBRS_DIR)/src

# Shim includes (compat headers for saver code)
SHIM_INC = -I$(INCS_DIR)/shim

# Optimizer options
OPT_ZERO = -O0 -g
OPT_TWO = -DNDEBUG -O2
OPT = $(OPT_TWO)
EM_OPT = $(OPT_TWO)

# ASYNCIFY: allow the unmodified native main loop to yield to the browser
EM_ASYNCIFY = -sASYNCIFY -sASYNCIFY_STACK_SIZE=65536

# Heap: savers allocate large textures/particle buffers (plasma needs ~40MB),
# so allow the wasm heap to grow rather than fixing a small initial size.
# GL_MAX_TEMP_BUFFER_SIZE: gl4es batches immediate-mode geometry into large
# client-side vertex arrays; emscripten's FULL_ES2 path copies these through a
# temp VBO whose default max (2MB) is too small for big draws (hyperspace's
# starfield), causing an out-of-range crash. 32MB comfortably covers them.
EM_MEMORY = -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=67108864 -sMAXIMUM_MEMORY=268435456 \
            -sGL_MAX_TEMP_BUFFER_SIZE=33554432

# Debug options
EXTRA_DEBUG =
EM_EXTRA_DEBUG = -s ASSERTIONS=2 -s SAFE_HEAP=1 -s STACK_OVERFLOW_CHECK=2

# Base compilers
CC = cc $(EXTRA_DEBUG)
CXX = c++ $(EXTRA_DEBUG)
EMCC = emcc -s WASM=1
EMCXX = em++ -s WASM=1

# Library archivers
AR = ar rvsc
EMAR = emar rvsc

# Compiler options for original saver code (C++98-era, warning-noisy)
SAVER_CODE_CXX = $(CXX) -std=gnu++14
SAVER_CODE_EMCXX = $(EMCXX) -std=gnu++14
SAVER_CODE_WARN_OFF = -Wno-deprecated-declarations -Wno-writable-strings \
	-Wno-write-strings -Wno-parentheses -Wno-comment -Wno-narrowing \
	-Wno-unused-value -Wno-unused-variable -Wno-char-subscripts \
	-Wno-deprecated-non-prototype -Wno-unused-command-line-argument \
	$(SAVER_CODE_WARN_OFF_EXTRA)
EM_SAVER_CODE_WARN_OFF = $(SAVER_CODE_WARN_OFF)

# Compiler options for new code (librs shell etc.)
MODERN_CODE_CC = $(CC) -std=gnu17
MODERN_CODE_CXX = $(CXX) -std=gnu++17
MODERN_CODE_EMCC = $(EMCC) -std=gnu17
MODERN_CODE_EMCXX = $(EMCXX) -std=gnu++17
