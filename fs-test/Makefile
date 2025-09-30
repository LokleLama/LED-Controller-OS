# Makefile for building all C/C++ source files in this directory and subdirectories

# Compiler and flags
CC := gcc
CXX := g++
CFLAGS := -Wall -Wextra -O2
CXXFLAGS := -Wall -Wextra -O2


# Find all source files
SRC_C := $(shell find . -name '*.c')
SRC_CPP := $(shell find . -name '*.cpp')
# Place all object files in obj/ directory, preserving relative paths
OBJ := $(patsubst ./%,obj/%.o,$(basename $(SRC_C))) $(patsubst ./%,obj/%.o,$(basename $(SRC_CPP)))

# Find all include files
INCLUDE_FILES := $(shell find . -name '*.h' -o -name '*.hpp')
INCLUDES := $(patsubst %,-I%,$(sort $(dir $(INCLUDE_FILES))))

# Output binary
TARGET := fs-test


# Ensure obj directory exists before building
all: objdir $(TARGET)

# Create obj directory
objdir:
	@mkdir -p obj


# Link object files
$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $@


# Compile C sources into obj/
obj/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile C++ sources into obj/
obj/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@


# Clean rule
clean:
	rm -rf obj $(TARGET)

.PHONY: all clean