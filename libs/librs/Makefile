LIBNAME = rs
LIB_SRC = src/rsSDLSaver.cpp
# Common includes (no host SDL here so the web build uses emscripten's SDL port)
LIB_CFLAGS = -Isrc $(GL4ES_INC) -I$(GL4ES_DIR)/include -I$(RSMATH_DIR)/src
# Native: host SDL2 headers via sdl2-config
NATIVE_LIB_CFLAGS = $(SDL_INC)
# Web: SDL2 comes from emscripten's USE_SDL port
EM_LIB_CFLAGS = $(EM_SDL_LIBS)
include ../../makefiles/make_lib.mk
