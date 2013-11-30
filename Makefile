$(shell sh build.sh 1>&2)
include build.mk

.PHONY: all tools clean

all:
	cd util; make
	cd comet; make

tools:
	cd tools; make

clean:
	rm -f *.exe.stackdump
	cd util; make clean
	cd comet; make clean
clean_all: clean
	rm -f deps/jemalloc-3.4.0/Makefile deps/libevent-2.0.21-stable/Makefile 
	
