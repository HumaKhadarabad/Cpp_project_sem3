CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pthread

LIB_SRC = Simulator.cpp Output.cpp
HEADERS = Wire.h Component.h Gates.h Circuits.h SimulationResult.h Simulator.h Output.h

all: circuit_designer tests

circuit_designer: main.cpp $(LIB_SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) main.cpp $(LIB_SRC) -o $@

tests: tests.cpp $(LIB_SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) tests.cpp $(LIB_SRC) -o $@

run: circuit_designer
	./circuit_designer 4

test: tests
	./tests

clean:
	rm -f circuit_designer tests *.csv
