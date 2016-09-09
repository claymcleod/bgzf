CC=gcc
CFLAGS=-std=c99
INCLUDE=-Iinclude/
LIBS=lib/libz.a
all:
	$(CC) $(CFLAGS) $(INCLUDE) $(LIBS) bgzf/bgzf_read.c -o bin/bgzf_read
