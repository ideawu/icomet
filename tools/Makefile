include ../config.mk


all:
	$(CC) -o ./benchmark \
		$(CFLAGS) \
		-I../src \
		../src/util/*.o benchmark.cpp \
		$(LIBEVENT_PATH)/.libs/libevent.a

clean:
	rm -f *.o
