CC=clang
CFLAGS=-Wall -Werror -Os -fno-unwind-tables -fomit-frame-pointer -fno-exceptions -fno-rtti -flto -std=c11
LDFLAGS=-flto

beedb: main.o book.o page.o port_pipe.o port_udp.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *.o beedb

.PHONY: clean
