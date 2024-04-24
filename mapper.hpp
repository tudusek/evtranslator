#include <cstdlib>
#include <iostream>
#include <ostream>
#include <fstream>
#include <string>
#include <vector>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <nlohmann/json.hpp>
#include <linux/input-event-codes.h>
#include <nlohmann/json_fwd.hpp>

using json = nlohmann::json;

class evdevMapper {
public:
  evdevMapper(int argc, char* argv[]); 
  ~evdevMapper();
private:
  std::string in_device_path = "";
  json data;
  struct libevdev *input_dev = nullptr;
  std::vector<struct libevdev *> output_devs;
  std::vector<struct libevdev_uinput *> output_devs_uinput;
  void parseParameters(int argc, char* argv[]);
  void setupInputDev();
  void setupOutputDev();
  void eventLoop();
};
