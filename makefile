CC = gcc

.PHONY: all clean

all: INETserver INETclient UNIXserver UNIXclient

INETserver: INETserver.c
	@echo Compiling INETserver.c
	$(CC) -o INETserver INETserver.c -ljansson

INETclient: INETapp.c
	@echo Compiling INETapp.c
	$(CC) `pkg-config --cflags gtk+-3.0` -o INETclient INETapp.c `pkg-config --libs gtk+-3.0` -ljansson

UNIXserver: UNIXserver.c
	@echo Compiling UNIXserver.c
	$(CC) -o UNIXserver UNIXserver.c -ljansson

UNIXclient: UNIXapp.c
	@echo Compiling UNIXclient.c
	$(CC) `pkg-config --cflags gtk+-3.0` -o UNIXclient UNIXapp.c `pkg-config --libs gtk+-3.0` -ljansson

	
clean:
	rm -f app1 app2

.PHONY: clean