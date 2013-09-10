include ../../build_config.mk

CFLAGS += ${PLATFORM_CFLAGS}
OBJS = log.o config.o
EXES = 

all: ${OBJS}

log.o: log.h log.cpp
	g++ ${CFLAGS} -c log.cpp

config.o: config.h config.cpp
	g++ ${CFLAGS} -c config.cpp

clean:
	rm -f ${EXES} ${OBJS}

