ifndef MAKEFILE_GLIB
MAKEFILE_GLIB = 1

GLIB_BUILD_DIR = $(DEPS_BIN_DIR)/glib_build
GLIB = $(DEPS_BIN_DIR)/lib/pkgconfig/glib-2.0.pc

$(GLIB): $(LIBFFI)
	mkdir -p "$(GLIB_BUILD_DIR)"
	CFLAGS= CXXFLAGS= LDFLAGS= $(MESON) "$(GLIB_BUILD_DIR)" "$(CURDIR)/deps/glib" \
	    --buildtype $(MESON_BUILD_TYPE) \
	    --prefix "$(DEPS_BIN_DIR)" \
	    --libdir lib \
	    --default-library static \
	    -Db_staticpic=true \
	    -Dinternal_pcre=true \
	    -Dnls=disabled
	$(NINJA) -C "$(GLIB_BUILD_DIR)" -j$(NUMPROC) install
	sed -i'.original' 's/^Libs:.*/& -lpthread/' $(GLIB)

.PHONY: glib
glib: $(GLIB)

.PHONY: deps
deps: glib

endif
