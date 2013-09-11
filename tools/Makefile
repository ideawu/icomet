include ../build_config.mk

all:
	g++ -g -o ./benchmark \
		-I${LIBEVENT_PATH} \
		-I${LIBEVENT_PATH}/compat \
		-I${LIBEVENT_PATH}/include \
		-I../src \
		${LIBEVENT_PATH}/.libs/libevent.a \
		../src/util/*.o benchmark.cpp
	g++ -g -o ./a \
		-I${LIBEVENT_PATH} \
		-I${LIBEVENT_PATH}/compat \
		-I${LIBEVENT_PATH}/include \
		-I../src \
		${LIBEVENT_PATH}/.libs/libevent.a \
		../src/util/*.o a.cpp

clean:
	rm -f *.o
