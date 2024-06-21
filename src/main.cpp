#include <iostream>
#include <cstring>
#include "../include/mapper.hpp"

int main (int argc, char *argv[]) {
  if (argc != 3 && argc != 4) {
    std::cout
        << "Usage: evdevmapper /path/to/device /path/to/configuration.lua [options]"
        << std::endl;
    return -1;
  }
  if(argc == 4 && !strcmp("--grab",argv[3])) {
    EvTranslator::grab = true;
  }
  
  EvTranslator::init(argv[1],argv[2]);
  return 0;
}
