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
LOCAL_INSTALL_DIR = ~/bin

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
BINS = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%, $(SRCS))

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
	@echo "Copied binary: $(LOCAL_INSTALL_DIR)/generate-prompt -> $(abspath $(BIN_DIR)/generate-prompt)"

clean:
	$(RM) -r $(BUILD_DIR) $(BINS)

debug: CFLAGS += -g  # Add -g flag for debugging
debug: build  # Build with debugging support


test:
	bats test


# No arguments, default to build
default: build
