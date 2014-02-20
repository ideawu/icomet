$(shell sh build.sh 1>&2)
include build.mk

.PHONY: all tools clean

all:
	cd src/util; make
	cd src/comet; make

tools:
	cd tools; make

clean:
	rm -f *.exe.stackdump
	cd src/util; make clean
	cd src/comet; make clean

clean_all: clean
	rm -f deps/jemalloc-3.4.0/Makefile deps/libevent-2.0.21-stable/Makefile 
	
