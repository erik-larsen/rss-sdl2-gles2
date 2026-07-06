APPNAME = skyrocket

# Skyrocket has positional fireworks audio via OpenAL. Sound is OPTIONAL:
# define RSS_USE_OPENAL and link OpenAL to enable it. Without it, the saver
# builds and runs muted (the SoundEngine becomes a silent stub).
#
# Native: enabled automatically if OpenAL is found via pkg-config.
# Browser: Emscripten ships an OpenAL implementation; enable explicitly.

OPENAL_CFLAGS := $(shell pkg-config --silence-errors --cflags openal)
OPENAL_LIBS   := $(shell pkg-config --silence-errors --libs openal)

ifneq ($(OPENAL_LIBS),)
  SAVER_EXTRA_CFLAGS = -DRSS_USE_OPENAL $(OPENAL_CFLAGS)
  SAVER_EXTRA_LIBS   = $(OPENAL_LIBS)
endif

# Emscripten: -lopenal pulls in emscripten's OpenAL; headers are at AL/al.h
EM_SAVER_EXTRA = -DRSS_USE_OPENAL -lopenal

include ../../makefiles/make_saver.mk
