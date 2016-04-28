CC=gcc
CFLAGS=-I.

makemyscroll: makemyscroll myscroll.c
	$(CC) -o makemyscroll myscroll.c $(CFLAGS)
