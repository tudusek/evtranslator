#include <iostream>
#include "../include/mapper.hpp"

int main (int argc, char *argv[]) {
  if (argc != 3) {
    std::cout
        << "Usage: evdevmapper /path/to/device /path/to/configuration.lua"
        << std::endl;
    return -1;
  }
  EvTranslator::init(argv[1],argv[2]);
  return 0;
}
