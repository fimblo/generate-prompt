# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lgit2

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
BIN = $(BIN_DIR)/git-prompt

# Targets
.PHONY: all build install clean

all: build

build: $(BIN)

$(BIN): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

install:
	@echo "Installing $(BIN) to /usr/local/bin"
	@install -m 755 $(BIN) /usr/local/bin/

clean:
	$(RM) -r $(BUILD_DIR) $(BIN)

# No arguments, default to build
default: build
