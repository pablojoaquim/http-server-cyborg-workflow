CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -pthread
TARGET := http_server
SRC := src/main.cpp src/server.cpp src/http_handler.cpp

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)
