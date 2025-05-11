CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread
LDFLAGS = -pthread -luuid -lresolv

# Target executables
SERVER = server

# Source files
SERVER_SRCS = server.cpp Tablet/tablet.cpp Util/iotool.cpp

# Object files
SERVER_OBJS = $(SERVER_SRCS:.cpp=.o)

# Default target
all: $(SERVER)

# Server executable
$(SERVER): $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Generic rule for building object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -f $(SERVER) *.o

# Run server with default settings
run_server:
	./$(SERVER)

.PHONY: all clean server