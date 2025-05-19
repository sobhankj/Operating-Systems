# Makefile to compile Bserver and Bclient

# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -Wall -g

# Targets
all: Bserver Bclient

# Compile Bserver.cpp
Bserver: Bserver.cpp
	$(CXX) $(CXXFLAGS) -o Bserver Bserver.cpp

# Compile Bclient.cpp
Bclient: Bclient.cpp
	$(CXX) $(CXXFLAGS) -o Bclient Bclient.cpp

# Clean up
clean:
	rm -f Bserver Bclient