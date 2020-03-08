CC = gcc
CFLAGS = -Os -Wall -g -lpthread

main:main.c
	$(CC) $^ -o $@ $(CFLAGS)

debug:mian.c
	$(CC) $^ -o $@ $(CFLAGS) -DDEBUG

clean:
	$(RM) .*.sw? main debug *.o

.PHONY:all clean
