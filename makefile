#declare variables
CC=gcc
CFLAGS=-Wall

recover: recover.c
	$(CC) -o $@ $^

clean:
	$(RM) *.o *~
