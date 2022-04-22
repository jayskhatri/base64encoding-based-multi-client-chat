CC=g++

CFLAGS=-c -Wall

all: server client

server: server.o 
	$(CC) server.o -o server

server.o: server.cpp
	$(CC) $(CFLAGS) server.cpp

client: client.o
	$(CC) client.o -o client

client.o: client.cpp
	$(CC) $(CFLAGS) client.cpp

clean:
	rm -rf *o server client