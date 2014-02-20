include ../../build.mk

OBJS = log.o config.o
EXES = 

all: ${OBJS}

log.o: log.h log.cpp
	g++ -c ${CFLAGS} log.cpp

config.o: config.h config.cpp
	g++ -c ${CFLAGS} config.cpp

clean:
	rm -f ${EXES} ${OBJS}

