CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread
LDFLAGS = -pthread -luuid -lresolv

# Target executables
SHL = shell

# Source files
SHL_SRCS = shell.cpp tablet.cpp

# Object files
SHL_OBJS = $(SHL_SRCS:.cpp=.o)

# Default target
all: $(SHL)

# Server executable
$(SHL): $(SHL_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Generic rule for building object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -f $(SHL) *.o

# Run server with default settings
run_shell:
	./$(SHL)

.PHONY: all clean shell