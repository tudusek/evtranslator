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

class evdevMapper {
public:
  enum { 
    PARSE_DEVICE_PATH = 1,
    PARSE_DEVICE_NAME,
    PARSE_DEVICE_CONFIG
  };
  typedef struct {
    __u16 in_event_type;
    __u16 in_event_code;
    __u16 out_event_type;
    __u16 out_event_code;
  } MAP;
  evdevMapper(int argc, char* argv[]); 
  ~evdevMapper();
private:
  std::vector<MAP> config;
  std::string in_device_path = "";
  std::string out_device_name = "";
  struct libevdev *input_dev = nullptr;
  struct libevdev *output_dev = nullptr;
  struct libevdev_uinput *output_dev_uinput = nullptr;
  MAP parseCFG(char *str);
  void parseParameters(int argc, char* argv[]);
  void setupInputDev();
  void setupOutputDev();
  void eventLoop();
};
