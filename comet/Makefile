include ../config.mk

OBJS = channel.o server.o
CFLAGS += -I ../

all: $(OBJS)
	$(CC) -o ../comet-server $(CFLAGS) \
		comet-server.cpp $(OBJS) \
		../util/*.o \
		$(LIBEVENT_PATH)/.libs/libevent.a \
		$(CLIBS)

channel.o: channel.h channel.cpp
	$(CC) -c $(CFLAGS) channel.cpp
server.o: server.h server.cpp
	$(CC) -c $(CFLAGS) server.cpp

clean:
	rm -f *.o ../comet-server
