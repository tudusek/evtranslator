#include <vector>
#include <string>

class AbsInfo {
 public:
  int evcode;
  int min;
  int max;
  int fuzz;
  int resolution;
};
class Event {
 public:
  int type;
  int code;
  int value;
};
class Map {
 public:
  int event_code;
  int deadzone;
  std::string action_type;
  std::vector<Event> events;
};
class Mappings {
 public:
  std::vector<Map> key_events; 
  std::vector<Map> rel_events; 
  std::vector<Map> abs_events; 
};
class Config {
public:
  struct libevdev * output_dev;
  struct libevdev_uinput * output_dev_uinput;
  std::string device_name;
  int device_bus;
  int device_vendor;
  int device_product;
  int device_version;
  std::vector<AbsInfo> abs_info;
  Mappings mappings;
};
