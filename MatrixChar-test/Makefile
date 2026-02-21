# Simple Makefile for matrix-text test program

CXX = g++
CXXFLAGS = -Wall -O2
TARGET = matrix-text
SRC = matrix-text.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
