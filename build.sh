#!/bin/sh
BASE_DIR=`pwd`
TARGET_OS=`uname -s`
JEMALLOC_PATH="$BASE_DIR/deps/jemalloc-3.3.1"
LIBEVENT_PATH="$BASE_DIR/deps/libevent-2.0.21-stable"

case "$TARGET_OS" in
    Darwin)
        PLATFORM_LDFLAGS="-pthread"
        ;;
    Linux)
        PLATFORM_LDFLAGS="-pthread"
        ;;
    CYGWIN_*)
        PLATFORM_LDFLAGS="-lpthread"
        ;;
    SunOS)
        PLATFORM_LIBS="-lpthread -lrt"
        ;;
    FreeBSD)
        PLATFORM_LIBS="-lpthread"
        ;;
    NetBSD)
        PLATFORM_LIBS="-lpthread -lgcc_s"
        ;;
    OpenBSD)
        PLATFORM_LDFLAGS="-pthread"
        ;;
    DragonFly)
        PLATFORM_LIBS="-lpthread"
        ;;
    OS_ANDROID_CROSSCOMPILE)
        PLATFORM_LDFLAGS=""  # All pthread features are in the Android C library
        ;;
    HP-UX)
        PLATFORM_LDFLAGS="-pthread"
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

rm -f build_config.mk
echo "LIBEVENT_PATH=$LIBEVENT_PATH" >> build_config.mk

