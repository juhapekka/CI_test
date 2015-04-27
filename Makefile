CC = gcc -Wall -pedantic -ansi -O0 -g

CFLAGS=$(shell pkg-config --cflags gl x11)
LIBS=$(shell pkg-config --libs gl x11)

all:
	$(CC) main.c -o colorindextest $(CFLAGS) $(LIBS)

clean:
	@echo Cleaning up...
	@rm colorindextest
	@echo Done.
