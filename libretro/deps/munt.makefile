ifndef MAKEFILE_MUNT
MAKEFILE_MUNT = 1

MUNT_BUILD_DIR = $(DEPS_BIN_DIR)/munt_build
MUNT = $(MUNT_BUILD_DIR)/libmt32emu.a
MUNT_INCFLAGS += -I$(MUNT_BUILD_DIR)/include
MUNT_LDFLAGS += -L$(MUNT_BUILD_DIR) -lmt32emu

$(MUNT):
	mkdir -p $(MUNT_BUILD_DIR)
	cd $(MUNT_BUILD_DIR) \
	&& $(CMAKE) \
	    -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
	    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
	    -DCMAKE_INSTALL_PREFIX="$(DEPS_BIN_DIR)" \
	    -DLIB_INSTALL_DIR=lib \
	    -Dlibmt32emu_C_INTERFACE=ON \
	    -Dlibmt32emu_SHARED=OFF \
	    -Dlibmt32emu_WITH_INTERNAL_RESAMPLER=ON \
	    $(EXTRA_CMAKE_FLAGS) \
	    "$(CURDIR)/deps/munt/mt32emu" \
	&& VERBOSE=1 $(CMAKE) --build . --config $(CMAKE_BUILD_TYPE) -j $(NUMPROC)

.PHONY: munt
munt: $(MUNT)

.PHONY: deps
deps: munt

endif
