$(shell sh build.sh 1>&2)
include build_config.mk

all:
	g++ -o comet \
		-I${LIBEVENT_PATH} \
		-I${LIBEVENT_PATH}/compat \
		-I${LIBEVENT_PATH}/include \
		${LIBEVENT_PATH}/.libs/libevent.a src/comet.cpp
	
clean:
	rm -f *.exe.stackdump
