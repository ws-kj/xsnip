CC=gcc
CFLAGS=-I. -lX11 -lpng
DEPS= 
OBJ = xsnip.c

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

xsnip: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
