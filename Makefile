CC = clang
CFLAGS = -std=c99 -O3 -Wall -Wextra -Wpedantic
LDFLAGS = -lcurl -ljson-c -lm
PREFIX = /usr/local
all: main.c
	$(CC) $(CFLAGS) $(LDFLAGS) main.c -o wotb
clean:
	rm wotb
install:
	install -m755 wotb $(PREFIX)/bin
uninstall:
	rm $(PREFIX)/bin/wotb
