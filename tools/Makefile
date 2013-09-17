include ../config.mk

CFLAGS += \
		-I$(LIBEVENT_PATH) \
		-I$(LIBEVENT_PATH)/compat \
		-I$(LIBEVENT_PATH)/include

all:
	$(CC) -o ./benchmark \
		$(CFLAGS) \
		$(PLATFORM_LDFLAGS) \
		-I../src \
		../src/util/*.o benchmark.cpp \
		$(LIBEVENT_PATH)/.libs/libevent.a

clean:
	rm -f *.o
