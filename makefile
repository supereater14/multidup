BIN_DIR=bin
LIB_DIR=lib
OBJ_DIR=obj
SRC_DIR=src

CC=gcc
CFLAGS=-O3 -Wall -Wextra -pedantic -Werror -I$(LIB_DIR)
LFLAGS=-lpthread

BINS=$(BIN_DIR)/multidup
LIBS=$(LIB_DIR)/dup_worker.h
OBJS=$(OBJ_DIR)/dup_worker.o

all: $(BINS) $(OBJS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(LIB_DIR)/%.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN_DIR)/%: $(SRC_DIR)/%.c $(OBJS)
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $< $(OBJS)

.PHONY: clean install remove

clean:
	rm -f $(BINS) $(OBJS)

install: $(BINS)
	cp $(BIN_DIR)/multidup /bin/

remove:
	rm -f /bin/multidup
