CC=gcc
CFLAGS=-std=c99 -Wall -Wno-parentheses -g3 -O0 -D_GNU_SOURCE -D_XOPEN_SOURCE=700
LDFLAGS=-lncurses -lpanel
SOURCES=mini.c
OBJECTS=mini.o

mini: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LDFLAGS) -o $@

clean:
	rm -f $(OBJECTS)

mini.o: mini.c
