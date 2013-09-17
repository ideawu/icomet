include ../config.mk

OBJS = channel.o server.o
CFLAGS += \
		-I$(LIBEVENT_PATH) \
		-I$(LIBEVENT_PATH)/compat \
		-I$(LIBEVENT_PATH)/include

all: $(OBJS)
	$(CC) -o ../icomet \
		$(CFLAGS) \
		$(LIBEVENT_PATH)/.libs/libevent.a \
		util/*.o *.o icomet.cpp

channel.o: channel.h channel.cpp
	$(CC) -c $(CFLAGS) channel.cpp
server.o: server.h server.cpp
	$(CC) -c $(CFLAGS) server.cpp


clean:
	rm -f *.o
