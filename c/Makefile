CPPFLAGS += -I .
CFLAGS = -Wall -Wextra -Werror -g -O2 -std=c11

all: test

test: build/smoltlv.o build/test.o
	mkdir -p build
	$(CC) $(CFLAGS) -o build/test build/smoltlv.o build/test.o

build/smoltlv.o: smoltlv.c smoltlv.h
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o build/smoltlv.o smoltlv.c

build/test.o: test.c smoltlv.h
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o build/test.o test.c

clean:
	rm -rf build