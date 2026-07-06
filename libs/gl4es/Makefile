# libs/gl4es/Makefile -- build gl4es as a static lib for native + emscripten.
#
# gl4es' own CMake writes the archive to ./lib/libGL.a, so each target builds
# in its own dir and the result is copied to a target-specific name that the
# demo links against (libGL-native.a / libGL-web.a).

include ../../makefiles/platform.mk

# gl4es is vendored third-party; these are the warning categories its sources
# trip on clang by default (benign: macro NO_LOADER redefined =1 vs empty,
# brace-less else, one !x&y precedence note). Silence just those so a rebuild
# is quiet without hiding anything in our own code.
GL4ES_QUIET = -DCMAKE_C_FLAGS="-Wno-macro-redefined -Wno-dangling-else -Wno-logical-not-parentheses -Wno-parentheses"

# Native: SDL provides a real GetProcAddress (ANGLE), and we manage init
# timing ourselves, so disable the loader and the auto-init constructor.
NATIVE_CFG = -DNOX11=ON -DNOEGL=ON -DSTATICLIB=ON \
             -DNO_LOADER=ON -DNO_INIT_CONSTRUCTOR=ON \
             -DCMAKE_BUILD_TYPE=RelWithDebInfo $(GL4ES_QUIET)

# Web: follow gl4es' documented Emscripten recipe. Crucially this keeps the
# loader ENABLED -- under FULL_ES2 the GLES entry points are statically
# linked, and SDL_GL_GetProcAddress can return null for them, so a NO_LOADER
# build has no way to reach GL and renders black. We still call
# initialize_gl4es() explicitly from the host before any GL call.
WEB_CFG    = -DNOX11=ON -DNOEGL=ON -DSTATICLIB=ON \
             -DCMAKE_BUILD_TYPE=RelWithDebInfo $(GL4ES_QUIET)

NATIVE_LIB = lib/libGL-native.a
WEB_LIB    = lib/libGL-web.a

# gl4es' CMake names the library target OPENGL32 on win32, GL elsewhere
# (the emscripten build is never win32, so web is always libGL.a).
ifeq ($(OS),win)
BUILT_NATIVE_LIB = lib/libOPENGL32.a
else
BUILT_NATIVE_LIB = lib/libGL.a
endif

all: native

native: $(NATIVE_LIB)

browser: $(WEB_LIB)


# Build via `cd <dir> && cmake --build .` rather than `make -C`: cmake's
# default generator is Ninja on MSYS2 (no Makefile in the build dir), and the
# `cd ... &&` forces a shell so PATH lookup skips emsdk's cmake/ *directory*
# (a bare `cmake --build ...` recipe line is exec'd directly by make, which
# resolves that directory first and dies with "Permission denied").
$(NATIVE_LIB):
	mkdir -p build-native
	cd build-native && cmake .. $(NATIVE_CFG)
	cd build-native && cmake --build . -j
	cp $(BUILT_NATIVE_LIB) $(NATIVE_LIB)
	@echo "BUILT: gl4es $(NATIVE_LIB)"

$(WEB_LIB):
	mkdir -p build-web
	cd build-web && emcmake cmake .. $(WEB_CFG)
	cd build-web && cmake --build . -j
	cp lib/libGL.a $(WEB_LIB)
	@echo "BUILT: gl4es $(WEB_LIB)"

clean:
	rm -rf build-native build-web lib/libGL.a lib/libOPENGL32.a $(NATIVE_LIB) $(WEB_LIB)

.PHONY: all native browser clean
