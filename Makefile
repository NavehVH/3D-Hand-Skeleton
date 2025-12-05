# Makefile
# Builds the C++ Hand Skeleton Viewer

CXX = g++
CXXFLAGS = -Wall -std=c++17 -Iinclude
LDFLAGS = -lGL -lGLU -lglut

# Source files
SRC = src/main.cpp src/hand_loader.cpp

# Target executable
TARGET = build/hand_skeleton

all: $(TARGET)

$(TARGET): $(SRC)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)