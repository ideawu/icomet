include ../build.mk


all:
	$(CXX) -o ./benchmark \
		$(CFLAGS) \
		-I../src \
		../src/util/*.o benchmark.cpp \
		$(LIBEVENT_PATH)/.libs/libevent.a \
		$(CLIBS)

clean:
	rm -f *.o
