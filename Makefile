CC=gcc
CFLAGS=-c -Wall
LIBS=-lX11 -lcairo
PREFIX=

all: snowcloud

snowcloud: snowcloud.o gfx.o
	$(CC) snowcloud.o gfx.o $(LIBS) -o snowcloud

snowcloud.o:
	$(CC) $(CFLAGS) snowcloud.c

gfx.o:
	$(CC) $(CFLAGS) gfx.c

install:
	cp snowcloud $(PREFIX)/bin/snowcloud

clean:
	rm *.o snowcloud

