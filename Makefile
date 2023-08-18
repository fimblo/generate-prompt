# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lgit2

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    CFLAGS += -I/opt/homebrew/include/
	LDFLAGS += -L/opt/homebrew/lib
endif
ifeq ($(UNAME_S),Linux)
    CFLAGS += -I/usr/include/
endif

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
BINS = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%, $(SRCS))

# Targets
.PHONY: all build install clean

all: build

build: $(BINS)

$(BIN_DIR)/%: $(BUILD_DIR)/%.o
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

install:
	@echo "Installing $(BIN) to /usr/local/bin"
	@install -m 755 $(BIN) /usr/local/bin/

clean:
	$(RM) -r $(BUILD_DIR) $(BINS)

# Build only src/git-prompt.c
git-prompt: $(BUILD_DIR)/git-prompt.o
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $(BIN_DIR)/git-prompt $(LDFLAGS)

# No arguments, default to build
default: build
