# Compiler and flags
CC = g++
CFLAGS = -Wall -Wextra -std=c++17

# Targets and files
TARGET = pad42
SRC = pad42.cpp

# Build target
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Clean up build files
clean:
	rm -f $(TARGET)

