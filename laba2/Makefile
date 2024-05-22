CC = gcc
CFLAGS = -W -Wall -Wextra -std=c11
.PHONY: clean

all: parent child

parent: parent.c action.h
	$(CC) $(CFLAGS) parent.c -o parent

child: child.c action.h
	$(CC) $(CFLAGS) child.c -o child

run:
	./parent /mnt/D/University/cs/sem4/OSaSP/laba3/child

clean: 
	rm parent child