#!/bin/bash
BASE_DIR=`pwd`
TARGET_OS=`uname -s`
JEMALLOC_PATH="$BASE_DIR/deps/jemalloc-3.4.0"
LIBEVENT_PATH="$BASE_DIR/deps/libevent-2.0.21-stable"

if test -z "$TARGET_OS"; then
	TARGET_OS=`uname -s`
fi
if test -z "$MAKE"; then
	MAKE=make
fi
if test -z "$CC"; then
	CC=gcc
fi
if test -z "$CXX"; then
	CXX=g++
fi

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

case "$TARGET_OS" in
	CYGWIN*|FreeBSD|OS_ANDROID_CROSSCOMPILE)
		echo "not using jemalloc on $TARGET_OS"
	;;
	*)
		DIR=`pwd`
		cd $JEMALLOC_PATH
		if [ ! -f Makefile ]; then
			echo ""
			echo "##### building jemalloc... #####"
			./configure
			make
			echo "##### building jemalloc finished #####"
			echo ""
		fi
		cd "$DIR"
	;;
esac


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
rm -f src/version.h
echo "#ifndef ICOMET_CONFIG_H" >> build.h
echo "#define ICOMET_HTTP_HEADER_SERVER \"icomet `cat version`\"" >> build.h
echo "#define ICOMET_VERSION \"`cat version`\"" >> build.h
echo "#endif" >> build.h
case "$TARGET_OS" in
	CYGWIN*|FreeBSD)
	;;
        OS_ANDROID_CROSSCOMPILE)
                echo "#define OS_ANDROID 1" >> build.h
        ;;
	*)
		echo "#include <stdlib.h>" >> build.h
		echo "#include <jemalloc/jemalloc.h>" >> build.h
	;;
esac


######### generate build.mk #########

rm -f build.mk

echo C=$C >> build.mk
echo CC=$CC >> build.mk
echo CXX=$CXX >> build.mk
echo CFLAGS := >> build.mk
echo CFLAGS += -O2 -Wall -Wno-sign-compare >> build.mk
echo CFLAGS += -D__STDC_FORMAT_MACROS >> build.mk
echo CFLAGS += -I \"$LIBEVENT_PATH\" >> build.mk
echo CFLAGS += -I \"$LIBEVENT_PATH/include\" >> build.mk
echo CFLAGS += -I \"$LIBEVENT_PATH/compact\" >> build.mk

echo CLIBS := >> build.mk
echo CLIBS += $PLATFORM_LIBS >> build.mk

case "$TARGET_OS" in
	CYGWIN*|FreeBSD|OS_ANDROID_CROSSCOMPILE)
	;;
	*)
		echo "CLIBS += \"$JEMALLOC_PATH/lib/libjemalloc.a\"" >> build.mk
		echo "CFLAGS += -I \"$JEMALLOC_PATH/include\"" >> build.mk
	;;
esac

echo LIBEVENT_PATH = $LIBEVENT_PATH >> build.mk
echo JEMALLOC_PATH = $JEMALLOC_PATH >> build.mk

