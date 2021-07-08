.RECIPEPREFIX +=
CC = gcc
CFLAGS = -Wall -lm -lrt -lwiringPi -lpthread

PROG = bin/*
OBJS = obj/*

default:
    mkdir -p bin obj
    $(CC) $(CFLAGS) -c src/main.c -o obj/main
    $(CC) $(CFLAGS) obj/main -o bin/main
    chmod 777 bin/main

run:
    sudo ./bin/main

clean:
    rm $(PROG) $(OBJS)

