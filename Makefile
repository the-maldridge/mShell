CC=gcc
CFLAGS=-Wall -Wextra -O0 -g
SOURCES=mshell.c
EXECNAME=mShell

all:
	$(CC) $(CFLAGS) $(SOURCES) -o $(EXECNAME)

test: all
	./$(EXECNAME)

valgrind: all
	valgrind ./$(EXECNAME)
