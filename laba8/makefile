#makefile
CC = gcc
CFLAGS = -W -Wall -std=c11 -pedantic -Wno-unused-parameter -Wno-unused-variable -pthread
.PHONY : clear

all : server client

server: server.c makefile
	$(CC) $(CFLAGS) server.c -o server

client: client.c makefile
	$(CC) $(CFLAGS) client.c -o client

clear:
	rm -f server client

run_server:
	./server

run_client:
	./client