CC=gcc
CFLAGS=-std=c99 -Wall -Wno-parentheses -g3 -O0 -D_GNU_SOURCE -D_XOPEN_SOURCE=700
LDFLAGS=-lncursesw -lpanel
SOURCES=mini.c color.c
OBJECTS=mini.o color.o

mini: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LDFLAGS) -o $@

clean:
	rm -f $(OBJECTS) mini

mini.o: mini.c mini.h color.h
color.o: color.c color.h
