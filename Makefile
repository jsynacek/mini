CC=gcc
CFLAGS=-std=c99 -Wall -Wno-parentheses -g3 -O0 -D_GNU_SOURCE -D_XOPEN_SOURCE=700
LDFLAGS=-lncursesw -lpanel
SOURCES=mini.c color.c utf8.c
OBJECTS=mini.o color.o utf8.o

mini: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LDFLAGS) -o $@

clean:
	rm -f $(OBJECTS) mini

mini.o: mini.c mini.h color.h utf8.h
color.o: color.c color.h
