CC = gcc
CFLAGS = -Wall

bin/ls: src/ls-v1.0.0.c
	$(CC) $(CFLAGS) -o bin/ls src/ls-v1.0.0.c

clean:
	rm -f bin/ls
