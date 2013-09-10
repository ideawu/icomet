$(shell sh build.sh 1>&2)
include build_config.mk

all:
	cd src/util; make
	cd src; make
	
clean:
	rm -f *.exe.stackdump
	cd src/util; make clean
	cd src; make clean
