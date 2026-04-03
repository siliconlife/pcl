# Simple Makefile for GPU test
CXX = g++
CXXFLAGS = -std=c++14 -O2 \
    -I../../common/include \
    -I../../build_musa/include \
    -I../../gpu/containers/include \
    -I../../gpu/utils/include \
    -I/usr/include/eigen3

TARGET = test_gpu_simple
SOURCES = test_gpu_simple.cpp

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)

test: $(TARGET)
	./$(TARGET)

.PHONY: all clean test

