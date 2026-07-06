include ../../makefiles/platform.mk

# Expects (set before including this file):
#   APPNAME            - saver executable name, e.g. flux
#   SAVER_EXTRA_CFLAGS - optional extra compile flags
#   SAVER_EXTRA_LIBS   - optional extra native link libs (e.g. -lopenal)
#   EM_SAVER_EXTRA     - optional extra emcc link flags

APP = $(BIN_DIR)/$(APPNAME)
EM_APPNAME = $(WEB_DIR)/$(APPNAME)
EM_APP = $(EM_APPNAME).html

HDRS = $(wildcard *.h)
SRC = $(wildcard *.cpp)
OBJS = $(patsubst %.cpp,$(BIN_DIR)/%.o,$(SRC))
EM_OBJS = $(patsubst %.cpp,$(WEB_DIR)/%.o,$(SRC))

# Savers are built against the rsXScreenSaver code path, impersonated by librs
SAVER_DEFS = -DRS_XSCREENSAVER $(SAVER_EXTRA_CFLAGS)
SAVER_INC = $(LIBRS_INC) $(GL4ES_INC) $(GLUES_INC) $(RSMATH_INC) $(SHIM_INC)

all: native browser

native: $(APP)

browser: $(EM_APP)

# Library dependencies (build on demand)
$(GL4ES_LIB):
	make native -C $(GL4ES_DIR)
$(EM_GL4ES_LIB):
	make browser -C $(GL4ES_DIR)
$(GLUES_LIB):
	make native -C $(GLUES_DIR)
$(EM_GLUES_LIB):
	make browser -C $(GLUES_DIR)
$(RSMATH_LIB):
	make native -C $(RSMATH_DIR)
$(EM_RSMATH_LIB):
	make browser -C $(RSMATH_DIR)
$(LIBRS_LIB):
	make native -C $(LIBRS_DIR)
$(EM_LIBRS_LIB):
	make browser -C $(LIBRS_DIR)

$(BIN_DIR):
	mkdir -p $@
	echo "*" > $@/.gitignore

$(WEB_DIR):
	mkdir -p $@
	echo "*.o" > $@/.gitignore
	echo "*.html" >> $@/.gitignore
	echo "*.js" >> $@/.gitignore
	echo "*.wasm" >> $@/.gitignore

# Saver objects also depend on the librs public headers (the SDL shell shim,
# mini-GLUT, ARB-shader shim) so changes there trigger a rebuild.
LIBRS_HDRS = $(wildcard $(LIBRS_DIR)/src/*.h $(LIBRS_DIR)/src/*/*.h)

$(OBJS): $(BIN_DIR)/%.o: $(SRC_DIR)/%.cpp $(HDRS) $(LIBRS_HDRS) | $(BIN_DIR)
	$(SAVER_CODE_CXX) $(OPT) $(SAVER_CODE_WARN_OFF) $(SAVER_DEFS) $(SAVER_INC) $(SDL_INC) $< -c -o $@

$(APP): $(GL4ES_LIB) $(GLUES_LIB) $(RSMATH_LIB) $(LIBRS_LIB) $(OBJS)
	$(MODERN_CODE_CXX) $(OPT) $(OBJS) \
		$(LIBRS_LIB) $(RSMATH_LIB) $(GLUES_LIB) $(GL4ES_LIB) \
		$(SDL_LIBS) $(GLES_LIBS) $(GLES_LINK) \
		$(SAVER_EXTRA_LIBS) -lm $(CONSOLE_FLAGS) -o $@
	$(call GLES_INSTALL)
	@echo
	@echo BUILT: $@

$(EM_OBJS): $(WEB_DIR)/%.o: $(SRC_DIR)/%.cpp $(HDRS) $(LIBRS_HDRS) | $(WEB_DIR)
	$(SAVER_CODE_EMCXX) $(EM_OPT) $(EM_SAVER_CODE_WARN_OFF) $(SAVER_DEFS) $(SAVER_INC) $(EM_SDL_LIBS) $< -c -o $@

# Custom HTML shell (fullscreen black canvas) replacing emscripten's default
EM_SHELL = --shell-file $(LIBRS_DIR)/shell.html

$(EM_APP): $(EM_GL4ES_LIB) $(EM_GLUES_LIB) $(EM_RSMATH_LIB) $(EM_LIBRS_LIB) $(EM_OBJS)
	$(MODERN_CODE_EMCXX) $(EM_OPT) $(EM_OBJS) \
		$(EM_LIBRS_LIB) $(EM_RSMATH_LIB) $(EM_GLUES_LIB) $(EM_GL4ES_LIB) \
		$(EM_SDL_LIBS) $(EM_ASYNCIFY) $(EM_MEMORY) $(EM_SHELL) $(EM_SAVER_EXTRA) -lm -o $@
	@echo
	@echo BUILT: $@

run: run-native

run-native: native
	$(APP) --window 1024x640 $(APPARGS)

run-browser: browser
	emrun $(EM_APP)

clean:
	rm -rf $(BIN_DIR) $(WEB_DIR)

.PHONY: all native browser run run-native run-browser clean
