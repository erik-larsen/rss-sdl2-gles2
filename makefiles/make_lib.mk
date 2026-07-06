include ../../makefiles/platform.mk

# Expects (set before including this file):
#   LIBNAME      - e.g. rsmath
#   LIB_SRC      - list of .c/.cpp sources
#   LIB_CFLAGS   - extra compile flags / includes
#   LIB_IS_SAVER_CODE - if 'yes', use the lenient legacy-code compiler settings

LIB = $(BIN_DIR)/lib$(LIBNAME).a
EM_LIB = $(WEB_DIR)/lib$(LIBNAME).a

C_SRC = $(filter %.c,$(LIB_SRC))
CXX_SRC = $(filter %.cpp,$(LIB_SRC))
OBJS = $(patsubst %.c,$(BIN_DIR)/%.o,$(notdir $(C_SRC))) \
       $(patsubst %.cpp,$(BIN_DIR)/%.o,$(notdir $(CXX_SRC)))
EM_OBJS = $(patsubst %.c,$(WEB_DIR)/%.o,$(notdir $(C_SRC))) \
          $(patsubst %.cpp,$(WEB_DIR)/%.o,$(notdir $(CXX_SRC)))

VPATH = $(sort $(dir $(LIB_SRC)))

ifeq ($(LIB_IS_SAVER_CODE),yes)
	LIB_CC = $(MODERN_CODE_CC) $(SAVER_CODE_WARN_OFF)
	LIB_CXX = $(SAVER_CODE_CXX) $(SAVER_CODE_WARN_OFF)
	EM_LIB_CC = $(MODERN_CODE_EMCC) $(EM_SAVER_CODE_WARN_OFF)
	EM_LIB_CXX = $(SAVER_CODE_EMCXX) $(EM_SAVER_CODE_WARN_OFF)
else
	LIB_CC = $(MODERN_CODE_CC)
	LIB_CXX = $(MODERN_CODE_CXX)
	EM_LIB_CC = $(MODERN_CODE_EMCC)
	EM_LIB_CXX = $(MODERN_CODE_EMCXX)
endif

all: native

native: $(LIB)

browser: $(EM_LIB)

$(BIN_DIR):
	mkdir -p $@
	echo "*" > $@/.gitignore

$(WEB_DIR):
	mkdir -p $@
	echo "*" > $@/.gitignore

$(BIN_DIR)/%.o: %.c | $(BIN_DIR)
	$(LIB_CC) $(OPT) $(LIB_CFLAGS) $(NATIVE_LIB_CFLAGS) $< -c -o $@

$(BIN_DIR)/%.o: %.cpp | $(BIN_DIR)
	$(LIB_CXX) $(OPT) $(LIB_CFLAGS) $(NATIVE_LIB_CFLAGS) $< -c -o $@

$(WEB_DIR)/%.o: %.c | $(WEB_DIR)
	$(EM_LIB_CC) $(EM_OPT) $(LIB_CFLAGS) $(EM_LIB_CFLAGS) $< -c -o $@

$(WEB_DIR)/%.o: %.cpp | $(WEB_DIR)
	$(EM_LIB_CXX) $(EM_OPT) $(LIB_CFLAGS) $(EM_LIB_CFLAGS) $< -c -o $@

$(LIB): $(OBJS)
	$(AR) $@ $(OBJS)
	@echo
	@echo BUILT: $@

$(EM_LIB): $(EM_OBJS)
	$(EMAR) $@ $(EM_OBJS)
	@echo
	@echo BUILT: $@

clean:
	rm -rf $(BIN_DIR) $(WEB_DIR)

.PHONY: all native browser clean
