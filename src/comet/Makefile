include ../../build.mk

OBJS = subscriber.o presence.o channel.o server.o
CFLAGS += -I ../

all: $(OBJS)
	${CXX} -o ../../icomet-server $(CFLAGS) \
		icomet-server.cpp $(OBJS) \
		../util/libutil.a \
		$(LIBEVENT_PATH)/.libs/libevent.a \
		$(CLIBS)

subscriber.o: subscriber.h subscriber.cpp
	${CXX} -c $(CFLAGS) subscriber.cpp
presence.o: presence.h presence.cpp
	${CXX} -c $(CFLAGS) presence.cpp
channel.o: channel.h channel.cpp
	${CXX} -c $(CFLAGS) channel.cpp
server.o: server.h server.cpp
	${CXX} -c $(CFLAGS) server.cpp

clean:
	rm -f *.o ../../icomet-server

