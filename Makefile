CC = clang
CFLAGS = -std=c99 -O3 -Wall -Wextra -Wpedantic
LDFLAGS = -lcurl -ljson-c -lm
PREFIX = /usr/local
all: main.c
	$(CC) $(CFLAGS) $(LDFLAGS) main.c -o wotbxp
clean:
	rm wotbxp
install:
	install -m755 wotbxp $(PREFIX)/bin
uninstall:
	rm $(PREFIX)/bin/wotbxp
