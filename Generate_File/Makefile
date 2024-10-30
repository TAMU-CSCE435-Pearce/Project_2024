# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -std=c++11  # Add your compiler flags here

# Target executable
TARGET = generate_file

# Source files
SRC = generate_file.cpp

# Object files
OBJ = $(SRC:.cpp=.o)

# Default target (run when you type 'make')
all: $(TARGET)

# Rule to create the executable
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)

# Rule to create the object file from the source file
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run the program
run: $(TARGET)
	./$(TARGET)

# Clean up the build
clean:
	rm -f $(OBJ) $(TARGET)
