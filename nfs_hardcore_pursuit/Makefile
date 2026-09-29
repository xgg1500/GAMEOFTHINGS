CXX ?= clang++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -Iinclude
SRC = src/main.cpp
TARGET = pursuit_sim

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
