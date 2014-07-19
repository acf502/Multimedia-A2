# Makefile for Assignment 2
CC=gcc
CFLAGS= -g -c -Wall `sdl-config --cflags`
LDFLAGS= `sdl-config --libs`
SOURCES=histEqImage.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=histEqImage

all: $(SOURCES) $(EXECUTABLE)
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
.c.o:
	$(CC) $(CFLAGS) $< -o $@
clean:
	rm *.o
	rm $(EXECUTABLE)
