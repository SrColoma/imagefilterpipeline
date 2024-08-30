CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lrt -lpthread

TARGETS = main publicador desenfocador realzador combinador
OBJS = bmp.o

all: $(TARGETS)

main: main.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

publicador: publicador.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

desenfocador: desenfocador.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

realzador: realzador.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

combinador: combinador.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

bmp.o: bmp.c bmp.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGETS) $(OBJS) *.o

.PHONY: all clean