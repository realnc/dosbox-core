ifndef MAKEFILE_FLUIDSYNTH
MAKEFILE_FLUIDSYNTH = 1

FLUIDSYNTH_BUILD_DIR = $(DEPS_BIN_DIR)/fluidsynth_build
FLUIDSYNTH = $(DEPS_BIN_DIR)/lib/pkgconfig/fluidsynth.pc
EXTRA_PACKAGES := fluidsynth $(EXTRA_PACKAGES)
COMMONFLAGS += -DWITH_FLUIDSYNTH

# Always a release build because debug build creates broken .pc file with mingw.
$(FLUIDSYNTH): $(LIBSNDFILE) $(LIBINSTPATCH) $(GLIB)
	mkdir -p "$(FLUIDSYNTH_BUILD_DIR)"
	cd "$(FLUIDSYNTH_BUILD_DIR)" \
	&& $(CMAKE) \
	    -DCMAKE_BUILD_TYPE=Release \
	    -DBUILD_SHARED_LIBS=OFF \
	    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
	    -DCMAKE_INSTALL_PREFIX="$(DEPS_BIN_DIR)" \
	    -DLIB_INSTALL_DIR=lib \
	    -DDEFAULT_SOUNDFONT= \
	    -Denable-alsa=OFF \
	    -Denable-aufile=OFF \
	    -Denable-dbus=OFF \
	    -Denable-dsound=OFF \
	    -Denable-ipv6=OFF \
	    -Denable-jack=OFF \
	    -Denable-ladspa=OFF \
	    -Denable-lash=OFF \
	    -Denable-libinstpatch=ON \
	    -Denable-libsndfile=ON \
	    -Denable-midishare=OFF \
	    -Denable-network=OFF \
	    -Denable-oboe=OFF \
	    -Denable-opensles=OFF \
	    -Denable-oss=OFF \
	    -Denable-pkgconfig=ON \
	    -Denable-portaudio=OFF \
	    -Denable-pulseaudio=OFF \
	    -Denable-readline=OFF \
	    -Denable-sdl2=OFF \
	    -Denable-systemd=OFF \
	    -Denable-threads=ON \
	    -Denable-waveout=OFF \
	    -Denable-winmidi=OFF \
	    -Denable-coreaudio=OFF \
	    -Denable-coremidi=OFF \
	    -Denable-framework=OFF \
	    $(EXTRA_CMAKE_FLAGS) \
	    "$(CURDIR)/deps/fluidsynth" \
	&& VERBOSE=1 $(CMAKE) --build . --config Release --target install -j $(NUMPROC)

.PHONY: fluidsynth
fluidsynth: $(FLUIDSYNTH)

.PHONY: deps
deps: fluidsynth

endif

