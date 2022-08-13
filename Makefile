CC = clang
CFLAGS = -std=c99 -O3 -Wall -Wextra -Wpedantic -s -pipe
LDLIBS = -lcurl -ljson-c
PREFIX = /usr/local
all: main.c
	$(CC) $(CFLAGS) $(LDLIBS) main.c -o wotb
clean:
	rm wotb
install:
	install -m755 wotb $(PREFIX)/bin
uninstall:
	rm $(PREFIX)/bin/wotb
