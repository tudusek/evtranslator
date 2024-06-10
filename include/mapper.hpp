#pragma once
#include <libevdev/libevdev-uinput.h>
#include <libevdev/libevdev.h>
#include <linux/input-event-codes.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <lua.hpp>
#include <string>
#include <vector>

class EvTranslator {
public:
  class Device {
  public:
    class EventClass {
    public:
      int eventType;
      std::vector<int> eventCodes;
    };
    struct libevdev *output_dev;
    struct libevdev_uinput *output_dev_uinput;
    std::string device_name;
    int device_bus;
    int device_vendor;
    int device_product;
    int device_version;
    std::vector<EventClass> advertisedCodes;
  };
  EvTranslator(std::string inputDevPath, std::string configPath);
  ~EvTranslator();

private:
  bool run = true;
  lua_State *L;
  std::string in_device_path;
  std::vector<Device> devices;
  struct libevdev *input_dev;
  void setupLua(std::string configPath);
  void setupInputDev();
  void setupOutputDev();
  void eventLoop();
};
