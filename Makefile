CC=gcc
CFLAGS=-c -Wall
LFLAGS=

all: shell

shell: shell.o filesys.o
	$(CC) $(LFLAGS) -o shell shell.o filesys.o

shell.o: shell.c 
	$(CC) $(CFLAGS) shell.c

filesys.o: filesys.c filesys.h
	$(CC) $(CFLAGS) filesys.c

clean:
	rm -f shell
	rm -f *.o
