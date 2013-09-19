include ../config.mk

OBJS = channel.o server.o

all: $(OBJS)
	$(CC) -o ../icomet $(CFLAGS) \
		util/*.o *.o icomet.cpp \
		$(LIBEVENT_PATH)/.libs/libevent.a \
		$(CLIBS)

channel.o: channel.h channel.cpp
	$(CC) -c $(CFLAGS) channel.cpp
server.o: server.h server.cpp
	$(CC) -c $(CFLAGS) server.cpp


clean:
	rm -f *.o
