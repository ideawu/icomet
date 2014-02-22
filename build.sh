#!/bin/bash
BASE_DIR=`pwd`
TARGET_OS=`uname -s`
JEMALLOC_PATH="$BASE_DIR/deps/jemalloc-3.4.0"
LIBEVENT_PATH="$BASE_DIR/deps/libevent-2.0.21-stable"
C=gcc
CC=g++

case "$TARGET_OS" in
    Darwin)
        ;;
    Linux)
        PLATFORM_LIBS="-lrt -pthread"
        ;;
    CYGWIN_*)
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
        ;;
    DragonFly)
        PLATFORM_LIBS=""
        ;;
    OS_ANDROID_CROSSCOMPILE)
        ;;
    HP-UX)
        ;;
    *)
        echo "Unknown platform!" >&2
        exit 1
esac


######### build jemalloc #########

if [[ $TARGET_OS == CYGWIN* ]]; then
	echo "not using jemalloc on $TARGET_OS"
else
	echo ""
	DIR=`pwd`
	cd "$JEMALLOC_PATH"
	if [ ! -f Makefile ]; then
		echo "building jemalloc..."
		./configure
		make
		echo "building jemalloc finished"
	fi
	cd "$DIR"
	echo ""
fi


######### build libevent #########

DIR=`pwd`
cd "$LIBEVENT_PATH"
if [ ! -f Makefile ]; then
	./configure
	make
fi
cd "$DIR"


######### generate build.h #########

rm -f build.h
echo "#ifndef ICOMET_CONFIG_H" >> build.h
echo "#define ICOMET_VERSION \"`cat version`\"" >> build.h
if [[ $TARGET_OS == CYGWIN* ]]; then
	:
else
	echo "#include <stdlib.h>" >> build.h
	echo "#include <jemalloc/jemalloc.h>" >> build.h
fi
echo "#endif" >> build.h


######### generate build.mk #########

rm -f build.mk

echo C=$C >> build.mk
echo CC=$CC >> build.mk
echo CFLAGS := >> build.mk
echo CFLAGS += -O2 -Wall -Wno-sign-compare >> build.mk
echo CFLAGS += -D__STDC_FORMAT_MACROS >> build.mk
echo CFLAGS += -I \"$LIBEVENT_PATH\" >> build.mk
echo CFLAGS += -I \"$LIBEVENT_PATH/include\" >> build.mk
echo CFLAGS += -I \"$LIBEVENT_PATH/compact\" >> build.mk

echo CLIBS := >> build.mk
echo CLIBS += $PLATFORM_LIBS >> build.mk

if [[ $TARGET_OS == CYGWIN* ]]; then
	:
else
	echo CFLAGS += -I \"$JEMALLOC_PATH/include\" >> build.mk
	echo CLIBS += \"$JEMALLOC_PATH/lib/libjemalloc.a\" >> build.mk
fi

echo LIBEVENT_PATH = $LIBEVENT_PATH >> build.mk
echo JEMALLOC_PATH = $JEMALLOC_PATH >> build.mk

