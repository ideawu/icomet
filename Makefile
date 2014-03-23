$(shell sh build.sh 1>&2)
include build.mk

.PHONY: all tools clean

all:
	mkdir -p logs
	cd src/util; make
	cd src/comet; make

tools:
	cd tools; make

clean:
	rm -f *.exe.stackdump
	cd src/util; make clean
	cd src/comet; make clean

clean_all: clean
	cd $(JEMALLOC_PATH); make clean
	cd $(LIBEVENT_PATH); make clean
	rm -f $(JEMALLOC_PATH)/Makefile $(LIBEVENT_PATH)/Makefile
	
