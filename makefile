CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread
LDFLAGS = -pthread -luuid -lresolv

# Target executables
SERVER = server
COORDINATOR = coordinator

# Source files
SERVER_SRCS = server.cpp Tablet/tablet.cpp Util/iotool.cpp
COORDINATOR_SRCS = coordinator.cpp Util/iotool.cpp

# Object files
SERVER_OBJS = $(SERVER_SRCS:.cpp=.o)
COORDINATOR_OBJS = $(COORDINATOR_SRCS:.cpp=.o)

# Default target
all: $(SERVER) $(COORDINATOR)

# Server executable
$(SERVER): $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Coordinator executable
$(COORDINATOR): $(COORDINATOR_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Generic rule for building object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -f $(SERVER) $(COORDINATOR) *.o Tablet/*.o Util/*.o

.PHONY: all clean