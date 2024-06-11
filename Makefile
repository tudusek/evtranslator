CXXFLAGS=
LIBS=-levdev -llua

all: src/mapper.cpp  src/main.cpp
	g++ $(CXXFLAGS) src/mapper.cpp src/main.cpp src/luaFunctions.cpp $(LIBS) -I/usr/include/libevdev-1.0/ -Iinclude/ -o evtranslator 
