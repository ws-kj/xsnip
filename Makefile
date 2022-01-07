CC=gcc
CFLAGS=-g -I. -lX11 -lpng
DEPS=config.h 
OBJ=xsnip.c

prefix=/usr/local
exec_prefix=${prefix}
bindir=${exec_prefix}/bin

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

xsnip: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

install:
	cp xsnip $(bindir)
