ifndef MAKEFILE_LIBFFI
MAKEFILE_LIBFFI = 1

LIBFFI_BUILD_DIR = $(DEPS_BIN_DIR)/libffi_build
LIBFFI = $(DEPS_BIN_DIR)/lib/pkgconfig/libffi.pc

$(LIBFFI):
	cd deps/libffi && ./autogen.sh
	mkdir -p "$(LIBFFI_BUILD_DIR)"
	cd "$(LIBFFI_BUILD_DIR)" \
	&& unset CFLAGS \
	&& unset CXXFLAGS \
	&& unset LDFLAGS \
	&& "$(CURDIR)/deps/libffi/configure" \
	    --host=$(TARGET_TRIPLET) \
	    --prefix="$(DEPS_BIN_DIR)" \
	    --disable-dependency-tracking \
	    --disable-shared \
	    --enable-static \
	    --enable-debug=$(AUTOCONF_DEBUG_FLAG) \
	    --enable-portable-binary \
	    --disable-docs \
	    --with-pic \
	    --with-gcc-arch=generic \
	&& $(MAKE) -j$(NUMPROC) install
	touch "$@"

.PHONY: libffi
libffi: $(LIBFFI)

.PHONY: deps
deps: libffi

endif
