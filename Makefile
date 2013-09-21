$(shell sh build.sh 1>&2)
include config.mk

.PHONY: all tools clean

all:
	cd src/util; make
	cd src; make

tools:
	cd tools; make

clean:
	rm -f *.exe.stackdump icomet
	cd src/util; make clean
	cd src; make clean
