CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra
TARGET := http_server
SRC := src/main.cpp

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)
