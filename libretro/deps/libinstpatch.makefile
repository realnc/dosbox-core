ifndef MAKEFILE_LIBINSTPATCH
MAKEFILE_LIBINSTPATCH = 1

LIBINSTPATCH_BUILD_DIR = $(DEPS_BIN_DIR)/libinstpatch_build
LIBINSTPATCH = $(DEPS_BIN_DIR)/lib/pkgconfig/libinstpatch-1.0.pc
EXTRA_PACKAGES := libinstpatch-1.0 $(EXTRA_PACKAGES)

# Always a release build because ubsan is forced in debug builds.
$(LIBINSTPATCH): $(LIBSNDFILE) $(GLIB)
	mkdir -p "$(LIBINSTPATCH_BUILD_DIR)"
	cd "$(LIBINSTPATCH_BUILD_DIR)" \
	&& CFLAGS= CXXFLAGS= LDFLAGS= $(CMAKE) \
	    -DCMAKE_BUILD_TYPE=Release \
	    -DBUILD_SHARED_LIBS=OFF \
	    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
	    -DCMAKE_INSTALL_PREFIX="$(DEPS_BIN_DIR)" \
	    -DLIB_INSTALL_DIR=lib \
	    $(EXTRA_CMAKE_FLAGS) \
	    "$(CURDIR)/deps/libinstpatch" \
	&& VERBOSE=1 $(CMAKE) --build . --config Release --target install -j $(NUMPROC)

.PHONY: libinstpatch
libinstpatch: $(LIBINSTPATCH)

.PHONY: deps
deps: libinstpatch

endif
