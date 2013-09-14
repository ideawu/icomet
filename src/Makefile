include ../config.mk

all:
	${CC} ${CFLAGS} \
		-c \
		-I${LIBEVENT_PATH} \
		-I${LIBEVENT_PATH}/compat \
		-I${LIBEVENT_PATH}/include \
		*.cpp
	${CC} ${CFLAGS} \
		-o ../icomet \
		-I${LIBEVENT_PATH} \
		-I${LIBEVENT_PATH}/compat \
		-I${LIBEVENT_PATH}/include \
		${LIBEVENT_PATH}/.libs/libevent.a \
		util/*.o *.o

clean:
	rm -f *.o
