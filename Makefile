CC=gcc
CFLAGS=-Wall -Wextra
SOURCES=mshell.c
EXECNAME=mShell

all:
	$(CC) $(CFLAGS) $(SOURCES) -o $(EXECNAME)

test: all
	./$(EXECNAME)
