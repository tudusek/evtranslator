
CXXFLAGS=
all: mapper.cpp mapper.hpp main.cpp
	g++ $(CXXFLAGS) mapper.cpp mapper.hpp main.cpp -levdev -o evdevmapper 
