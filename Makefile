# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra
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
LOCAL_INSTALL_DIR = ~/bin

# Source files
SRC = $(SRC_DIR)/generate-prompt.c
OBJ = $(BUILD_DIR)/generate-prompt.o
BIN = $(BIN_DIR)/generate-prompt

# Targets
.PHONY: all build install install-local clean test

all: build test

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

install-local: $(BINS)
	@mkdir -p $(LOCAL_INSTALL_DIR)
	@rm -f $(LOCAL_INSTALL_DIR)/generate-prompt
	@cp $(BIN_DIR)/generate-prompt $(LOCAL_INSTALL_DIR)/generate-prompt
	@echo "Copied binary: $(abspath $(BIN_DIR)/generate-prompt) -> $(LOCAL_INSTALL_DIR)/generate-prompt "

clean:
	$(RM) -r $(BUILD_DIR) $(BINS)

debug: CFLAGS += -g
debug: build
	@echo "Adding -g flag for debugging"



test:
	bats test


# No arguments, default to build
default: build
