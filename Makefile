CC = clang
CFLAGS = -O3 -Wall -std=c99 -Iinclude -pthread -g

SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
TARGET = heptagenerator

SOURCES := $(wildcard $(SRC_DIR)/*.c) main.c
OBJECTS := $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SRC_DIR)/*.o *.o $(TARGET)

.PHONY: all clean