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
