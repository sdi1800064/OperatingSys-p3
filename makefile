CC = gcc
CFLAGS = -Wall -Wextra -std=c11
LDFLAGS = -lrt -pthread

SRC_DIR = src
HEADERS_DIR = header
OBJ_DIR = obj
BUILD_DIR = .

# List of source files
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)

# List of object files
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))

# Targets
all: writer reader log

writer: $(OBJ_DIR)/writer.o
	$(CC) $< -o $(BUILD_DIR)/$@ $(LDFLAGS)

reader: $(OBJ_DIR)/reader.o
	$(CC) $< -o $(BUILD_DIR)/$@ $(LDFLAGS)

log: $(OBJ_DIR)/log.o
	$(CC) $< -o $(BUILD_DIR)/$@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS_DIR)/record.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(HEADERS_DIR) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR)/*.o writer reader log

.PHONY: all clean
