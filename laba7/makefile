#makefile
CC = gcc
CFLAGS = -W -Wall -std=c11 -pedantic -Wno-unused-parameter -Wno-unused-variable
.PHONY : clean

all : main generator

main: main.c makefile
	$(CC) $(CFLAGS) main.c -o main

generator: generator.c makefile
	$(CC) $(CFLAGS) generator.c -o generator

clean:
	rm -f main generator file

generate:
	./generator

run:
	./main