CC=gcc
CFLAGS=-O3 -Wall -Wextra -pedantic -Werror
LFLAGS=

BIN_DIR=bin
LIB_DIR=lib
OBJ_DIR=obj
SRC_DIR=src

BINS=
LIBS=
OBJS=

all: $(BINS) $(OBJS)

$(OBJ_DIR)/%.o: src/%.c lib/%.h
	$(CC) $(CFLAGS) $(LFLAGS) -c -o $@ $<

$(BIN_DIR)/%: src/%.c $(OBJS)
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $< $(OBJS)

.PHONY: clean

clean:
	rm -f $(BINS) $(OBJS)
