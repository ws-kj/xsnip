X11_CFLAGS != pkg-config --cflags x11
X11_LIBS != pkg-config --libs x11

LIBPNG_CFLAGS != pkg-config --cflags libpng
LIBPNG_LIBS != pkg-config --libs libpng

CC?=gcc
CFLAGS=-g ${X11_CFLAGS} ${LIBPNG_CFLAGS}
LIBS=${X11_LIBS} ${LIBPNG_LIBS}
DEPS=config.h 
OBJ=xsnip.o

prefix=/usr/local
exec_prefix=${prefix}
bindir=${exec_prefix}/bin

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

xsnip: $(OBJ)
	$(CC) -o $@ $^ $(LIBS)

install:
	cp xsnip $(bindir)
