CC=clang
CFLAGS=-framework CoreFoundation -framework IOKit -framework Carbon -g

all: powermated

powermated: main.c
	$(CC) $(CFLAGS) -o $@ $<
