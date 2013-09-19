#!/bin/sh
BASE_DIR=`pwd`
TARGET_OS=`uname -s`
JEMALLOC_PATH="$BASE_DIR/deps/jemalloc-3.3.1"
LIBEVENT_PATH="$BASE_DIR/deps/libevent-2.0.21-stable"
C=gcc
CC=g++

case "$TARGET_OS" in
    Darwin)
        PLATFORM_LDFLAGS=""
        ;;
    Linux)
        PLATFORM_LDFLAGS="-lrt"
        ;;
    CYGWIN_*)
        PLATFORM_LDFLAGS="-lrt"
        ;;
    SunOS)
        PLATFORM_LIBS="-lrt"
        ;;
    FreeBSD)
        PLATFORM_LIBS=""
        ;;
    NetBSD)
        PLATFORM_LIBS=" -lgcc_s"
        ;;
    OpenBSD)
        PLATFORM_LDFLAGS=""
        ;;
    DragonFly)
        PLATFORM_LIBS=""
        ;;
    OS_ANDROID_CROSSCOMPILE)
        PLATFORM_LDFLAGS=""  # All pthread features are in the Android C library
        ;;
    HP-UX)
        PLATFORM_LDFLAGS=""
        ;;
    *)
        echo "Unknown platform!" >&2
        exit 1
esac


DIR=`pwd`
cd $LIBEVENT_PATH
if [ ! -f Makefile ]; then
	./configure
	make
fi
cd "$DIR"

rm -f config.mk

echo C=$C >> config.mk
echo CC=$CC >> config.mk
echo CFLAGS := >> config.mk
echo CFLAGS += -g -O2 -Wall -Wno-sign-compare >> config.mk
echo PLATFORM_LDFLAGS := $PLATFORM_LDFLAGS >> config.mk
echo LIBEVENT_PATH = $LIBEVENT_PATH >> config.mk

