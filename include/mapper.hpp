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
    struct libevdev *output_dev;
    struct libevdev_uinput *output_dev_uinput;
  };
  static bool grab;

  static void init(std::string inputDevPath, std::string configPath);
  static void cleanup();
  static void handleSignal(int signal);

private:
  static bool run;
  static lua_State *L;
  static std::string in_device_path;
  static std::vector<Device> devices;
  static struct libevdev *input_dev;

  static void setupLua(std::string configPath);
  static void setupInputDev();
  static void setupOutputDev();
  static void eventLoop();

  static void addFunctionsToLua();
  class LuaFunctions {
  public:
    static int getABSInfo(lua_State *L);
    static int hasProperty(lua_State *L);
    static int hasEventType(lua_State *L);
    static int hasEventCode(lua_State *L);
  };
};
