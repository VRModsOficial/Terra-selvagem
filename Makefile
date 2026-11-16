.CC = gcc
CFLAGS = -Wall -Wextra -O2 $(shell pkg-config --cflags sdl2)
LDFLAGS = $(shell pkg-config --libs sdl2) -lGL -lm

SRC = src/main.c src/world.c src/terrain.c src/objloader.c src/audio.c
OUT = ilha

all: $(OUT)

$(OUT): $(SRC) src/world.h src/terrain.h src/objloader.h src/audio.h src/primitives.h
	$(CC) $(CFLAGS) -o $(OUT) $(SRC) $(LDFLAGS)

clean:
	rm -f $(OUT)

.PHONY: all clean
