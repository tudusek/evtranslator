CXXFLAGS= -Wall -Wextra
LIBS=-levdev -llua

all: 
	g++ $(CXXFLAGS) src/mapper.cpp src/main.cpp src/luaFunctions.cpp src/outputDevice.cpp $(LIBS) -I/usr/include/libevdev-1.0/ -Iinclude/ -o evtranslator 
