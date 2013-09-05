$(shell sh build.sh 1>&2)
include build_config.mk

all:
	gcc -o comet \
		-I${LIBEVENT_PATH} \
		-I${LIBEVENT_PATH}/compat \
		-I${LIBEVENT_PATH}/include \
		${LIBEVENT_PATH}/.libs/libevent.a comet.c
	
clean:
	rm -f *.exe.stackdump
