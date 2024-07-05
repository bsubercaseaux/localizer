CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lm

# Debug flags
DEBUG_FLAGS = -g -O0

# Output executable name
TARGET = realizer
DEBUG_TARGET = $(TARGET)_debug

# Main source file
MAIN = main.c solver.c

# Default target
all: $(TARGET)

# Debug target
debug: $(DEBUG_TARGET)

# Compiling and linking the target executable
$(TARGET): $(MAIN)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# Compiling and linking the debug version
$(DEBUG_TARGET): $(MAIN)
	$(CC) $(DEBUG_FLAGS) $(CFLAGS) $< -o $@ $(LDFLAGS)

# Run the program with GDB
gdb: $(DEBUG_TARGET)
	gdb ./$(DEBUG_TARGET)

lldb: $(DEBUG_TARGET)
	lldb ./$(DEBUG_TARGET)

# Clean up build artifacts
clean:
	rm -f $(TARGET) $(DEBUG_TARGET)

# Phony targets
.PHONY: all debug gdb lldb clean
