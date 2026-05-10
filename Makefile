CC = gcc
CFLAGS = -Wall -Wextra -g -02
LDFLAGS = -lpthread

SRC = $(wildcard src/*.c)
CONTROLLERS = $(wildcard controllers/*.c)
MODELS = $(wildcard models/*.c)
OBJ = $(SRC:.c=.0) $(CONTROLLERS:.c=.o) $(MODELS:.c=.o)

all: server

server: main.c $(OBJ)
			  $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c 
				$(CC) $(CFLAGS) -Iinclude -c -o $@ $<

clean:
				rm -rf server src/*.o controllers/*.o models/*.o 

run: server 
	     ./server

.PHONY: all clean run
