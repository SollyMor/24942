CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS = -lsocket -lnsl -lrt

all: server client

server: server.c
	$(CC) $(CFLAGS) -o server server.c $(LDFLAGS)

client: client.c
	$(CC) $(CFLAGS) -o client client.c $(LDFLAGS)

clean:
	rm -f server client socket30

.PHONY: all clean

