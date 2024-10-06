# Define the compiler and flags
CC = gcc
CFLAGS = -Iutils
LDFLAGS = -lreadline -lncurses

# Define the source and build directories
SRC_DIR = src
UTILS_DIR = utils
BUILD_DIR = build

# Define the output binaries and their corresponding source files
MAIN_BINARY = $(BUILD_DIR)/main
MAIN_SRC = $(SRC_DIR)/main.c $(UTILS_DIR)/utils.c

TUI_BINARY = $(BUILD_DIR)/file_manager
TUI_SRC = $(SRC_DIR)/ncur_ui.c

# Default target (run when no target is specified)
all: $(MAIN_BINARY) $(TUI_BINARY)

# Rule to compile the main binary
$(MAIN_BINARY): $(MAIN_SRC)
	$(CC) $(CFLAGS) -o $(MAIN_BINARY) $(MAIN_SRC) -lreadline

# Rule to compile the terminal user interface
$(TUI_BINARY): $(TUI_SRC)
	$(CC) -o $(TUI_BINARY) $(TUI_SRC) -lncurses

# Rule to install the main binary to /usr/local/bin
install: $(MAIN_BINARY)
	sudo mv $(MAIN_BINARY) /usr/local/bin/silica

# Rule to clean the build directory
clean:
	rm -rf $(BUILD_DIR)/*

# Ensure the build directory exists
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Phony targets (these don't correspond to real files)
.PHONY: all clean install

