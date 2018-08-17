LOCAL_PATH := $(call my-dir)
CORE_DIR   := $(LOCAL_PATH)/../..

INCFLAGS    := -I$(CORE_DIR)/libretro/deps/sdl/include/ -I$(CORE_DIR)/libretro/deps/sdl/include/SDL/ -I$(CORE_DIR)/libretro/deps/sdl/SDL_net/
COMMONFLAGS :=

WITH_DYNAREC :=
ifeq ($(TARGET_ARCH_ABI), armeabi)
    WITH_DYNAREC := oldarm
else ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
    WITH_DYNAREC := arm
else ifeq ($(TARGET_ARCH_ABI), arm64-v8a)
    WITH_DYNAREC := arm64
else ifeq ($(TARGET_ARCH_ABI), x86)
    WITH_DYNAREC := x86
else ifeq ($(TARGET_ARCH_ABI), x86_64)
    WITH_DYNAREC := x86_64
else ifeq ($(TARGET_ARCH_ABI), mips)
    WITH_DYNAREC := mips
else ifeq ($(TARGET_ARCH_ABI), mips64)
    WITH_DYNAREC := mips64
endif

WITH_IPX := 1

include $(CORE_DIR)/libretro/Makefile.common

SOURCES_C += \
    $(CORE_DIR)/libretro/deps/sdl/SDL/src/thread/pthread/SDL_syscond.c \
    $(CORE_DIR)/libretro/deps/sdl/SDL/src/thread/pthread/SDL_sysmutex.c \
    $(CORE_DIR)/libretro/deps/sdl/SDL/src/thread/pthread/SDL_syssem.c \
    $(CORE_DIR)/libretro/deps/sdl/SDL/src/thread/pthread/SDL_systhread.c \
    $(CORE_DIR)/libretro/deps/sdl/SDL/src/thread/SDL_thread.c \
    $(CORE_DIR)/libretro/deps/sdl/SDL/src/timer/unix/SDL_systimer.c \
    $(CORE_DIR)/libretro/deps/sdl/SDL/src/timer/SDL_timer.c \
    $(CORE_DIR)/libretro/deps/sdl/SDL/src/cdrom/dummy/SDL_syscdrom.c \
    $(CORE_DIR)/libretro/deps/sdl/SDL/src/cdrom/SDL_cdrom.c \
    $(CORE_DIR)/libretro/deps/sdl/SDL/src/SDL_error.c \
    $(CORE_DIR)/libretro/deps/sdl/SDL_net/SDLnet.c \
    $(CORE_DIR)/libretro/deps/sdl/SDL_net/SDLnetTCP.c \
    $(CORE_DIR)/libretro/deps/sdl/SDL_net/SDLnetUDP.c \
    $(CORE_DIR)/libretro/deps/sdl/SDL_net/SDLnetselect.c

SOURCES_CXX +=\
	$(CORE_DIR)/libretro/nonlibc/snprintf.cpp

COMMONFLAGS += -D__LIBRETRO__ -DFRONTEND_SUPPORTS_RGB565 $(INCFLAGS) -DC_HAVE_MPROTECT="1" -DC_IPX

GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"
SVN_VERSION := " $(shell cat ../svn)"

ifneq ($(GIT_VERSION)," unknown")
    COMMONFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

ifneq ($(SVN_VERSION)," unknown")
    COMMONFLAGS += -DSVN_VERSION=\"$(SVN_VERSION)\"
endif

include $(CLEAR_VARS)
LOCAL_MODULE       := retro
LOCAL_SRC_FILES    := $(SOURCES_C) $(SOURCES_CXX)
LOCAL_CFLAGS       := $(COMMONFLAGS)
LOCAL_CPPFLAGS     := $(COMMONFLAGS)
LOCAL_LDFLAGS      := -Wl,-version-script=$(CORE_DIR)/libretro/link.T
LOCAL_LDLIBS       := -llog
LOCAL_CPP_FEATURES := rtti exceptions
LOCAL_DISABLE_FATAL_LINKER_WARNINGS := true
include $(BUILD_SHARED_LIBRARY)
