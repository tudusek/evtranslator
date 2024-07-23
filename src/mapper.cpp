#include "../include/mapper.hpp"
#include <chrono>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <libevdev/libevdev-uinput.h>
#include <libevdev/libevdev.h>
#include <linux/input-event-codes.h>
#include <linux/input.h>
#include <lua.hpp>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

bool EvTranslator::run;
bool EvTranslator::grab = false;
lua_State *EvTranslator::L;
std::string EvTranslator::in_device_path;
std::vector<EvTranslator::Device> EvTranslator::devices;
struct libevdev *EvTranslator::input_dev;

void EvTranslator::setupLua(std::string configPath) {
  EvTranslator::L = luaL_newstate();
  luaL_openlibs(EvTranslator::L);
  addFunctionsToLua();
  // luaopen_base(EvTranslator::L);
  // luaopen_math(EvTranslator::L);
  // luaopen_utf8(EvTranslator::L);
  // luaopen_string(EvTranslator::L);
  // luaopen_package(EvTranslator::L);
  // luaopen_os(EvTranslator::L);
  // luaopen_table(EvTranslator::L);
  if (luaL_dofile(EvTranslator::L, configPath.c_str()) != LUA_OK) {
    std::cout << "[C] Error reading script\n";
    luaL_error(EvTranslator::L, "Error: %s\n",
               lua_tostring(EvTranslator::L, -1));
    EvTranslator::run = false;
    return;
  }
}

void EvTranslator::init(std::string inputDevPath, std::string configPath) {
  EvTranslator::run = true;
  EvTranslator::input_dev = nullptr;
  EvTranslator::in_device_path = inputDevPath;
  signal(SIGINT, EvTranslator::handleSignal);
  signal(SIGTERM, EvTranslator::handleSignal);

  setupInputDev();
  if (!EvTranslator::run)
    return;
  setupLua(configPath);
  if (!EvTranslator::run)
    return;
  setupOutputDev();
  if (!EvTranslator::run)
    return;
  eventLoop();
  cleanup();
}

void EvTranslator::handleSignal(int signal) { run = false; }

void EvTranslator::cleanup() {
  if (EvTranslator::L)
    lua_close(EvTranslator::L);

  if (input_dev) {
    int fd = libevdev_get_fd(input_dev);
    if (grab) {
      libevdev_grab(input_dev, LIBEVDEV_UNGRAB);
    }
    libevdev_free(input_dev);
    close(fd);
  }

  for (auto &device : EvTranslator::devices) {
    if (device.output_dev)
      libevdev_uinput_destroy(device.output_dev_uinput);
    if (device.output_dev_uinput)
      libevdev_free(device.output_dev);
  }
}

void EvTranslator::setupInputDev() {
  int input_dev_fd = open(in_device_path.c_str(), O_RDONLY);
  if (input_dev_fd < 0) {
    std::cout << "Failed to open input device" << std::endl;
    EvTranslator::run = false;
    return;
  }

  int error = libevdev_new_from_fd(input_dev_fd, &input_dev);
  if (error < 0) {
    std::cout << "Failed to init input device" << std::endl;
    EvTranslator::run = false;
    return;
  }

  if (grab) {
    // give user time to release keys
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    libevdev_grab(input_dev, LIBEVDEV_GRAB);
  }

  // std::cout << libevdev_get_name(input_dev) << std::endl;
}

void EvTranslator::setupOutputDev() {
  if (lua_getglobal(EvTranslator::L, "Devices") != LUA_TTABLE) {
    std::cout << "Output device(s) not specified" << std::endl;
    EvTranslator::run = false;
    return;
  }
  lua_pushnil(EvTranslator::L);
  while (lua_next(EvTranslator::L, -2) != 0) {
    EvTranslator::Device device;
    device.output_dev = libevdev_new();
    device.output_dev_uinput = nullptr;

    if (!device.output_dev) {
      std::cout << "Failed to allocate memory for device" << std::endl;
      EvTranslator::run = false;
      return;
    }

    if (!lua_istable(EvTranslator::L, -1)) {
      std::cout << "Not a table" << std::endl;
      EvTranslator::run = false;
      return;
    }

    if (lua_getfield(EvTranslator::L, -1, "deviceName") != LUA_TSTRING) {
      std::cout << "deviceName is not specified or is not a string"
                << std::endl;
      EvTranslator::run = false;
      return;
    } else {
      libevdev_set_name(device.output_dev, lua_tostring(EvTranslator::L, -1));
      std::cout << lua_tostring(EvTranslator::L, -1) << ":" << std::endl;
    }
    lua_pop(EvTranslator::L, 1);

    switch (lua_getfield(EvTranslator::L, -1, "advertise")) {
    case LUA_TTABLE:
      lua_pushnil(EvTranslator::L);
      while (lua_next(EvTranslator::L, -2) != 0) {
        if (!lua_isstring(EvTranslator::L, -2)) {
          std::cout << "Invalid event type in advertise" << std::endl;
          EvTranslator::run = false;
          return;
        }

        auto eventType =
            libevdev_event_type_from_name(lua_tostring(EvTranslator::L, -2));
        if (eventType == -1) {
          std::cout << "Invalid event type in advertise" << std::endl;
          EvTranslator::run = false;
          return;
        }

        std::cout << "\tEvent type: " << eventType << std::endl;
        lua_pushnil(EvTranslator::L);
        while (lua_next(EvTranslator::L, -2) != 0) {
          if (!lua_isstring(EvTranslator::L, -1)) {
            std::cout << "Invalid event code" << std::endl;
            EvTranslator::run = false;
            return;
          }

          auto evcode = libevdev_event_code_from_name(
              eventType, lua_tostring(EvTranslator::L, -1));

          if (evcode == -1) {
            std::cout << "Invalid event code" << std::endl;
            EvTranslator::run = false;
            return;
          }
          std::cout << "\t\tEvent code: " << evcode << std::endl;
          struct input_absinfo absInfo;
          switch (eventType) {
          case EV_ABS:
            if (lua_getfield(L, -6, "absInfo") != LUA_TTABLE) {
              std::cout << "Invalid event code" << std::endl;
              EvTranslator::run = false;
              return;
            }
            lua_getfield(L, -1, libevdev_event_code_get_name(EV_ABS, evcode));

            if (!lua_istable(L, -1)) {
              std::cout << "Invalid axis absInfo" << std::endl;
              EvTranslator::run = false;
              return;
            }

            if (lua_getfield(L, -1, "value") != LUA_TNUMBER) {
              std::cout << "Invalid absInfo value" << std::endl;
              EvTranslator::run = false;
              return;
            } else {
              absInfo.value = lua_tonumber(L, 1);
            }
            lua_pop(L, 1);

            if (lua_getfield(L, -1, "minimum") != LUA_TNUMBER) {
              std::cout << "Invalid absInfo minimum" << std::endl;
              EvTranslator::run = false;
              return;
            } else {
              absInfo.minimum = lua_tonumber(L, 1);
            }
            lua_pop(L, 1);

            if (lua_getfield(L, -1, "maximum") != LUA_TNUMBER) {
              std::cout << "Invalid absInfo maximum" << std::endl;
              EvTranslator::run = false;
              return;
            } else {
              absInfo.maximum = lua_tonumber(L, 1);
            }
            lua_pop(L, 1);

            if (lua_getfield(L, -1, "fuzz") != LUA_TNUMBER) {
              std::cout << "Invalid absInfo fuzz" << std::endl;
              EvTranslator::run = false;
              return;
            } else {
              absInfo.fuzz = lua_tonumber(L, 1);
            }
            lua_pop(L, 1);

            if (lua_getfield(L, -1, "flat") != LUA_TNUMBER) {
              std::cout << "Invalid absInfo fuzz" << std::endl;
              EvTranslator::run = false;
              return;
            } else {
              absInfo.flat = lua_tonumber(L, 1);
            }
            lua_pop(L, 1);

            if (lua_getfield(L, -1, "resolution") != LUA_TNUMBER) {
              std::cout << "Invalid absInfo fuzz" << std::endl;
              EvTranslator::run = false;
              return;
            } else {
              absInfo.resolution = lua_tonumber(L, 1);
            }
            lua_pop(L, 1);

            lua_pop(L, 2);
            libevdev_enable_event_code(device.output_dev, eventType, evcode,
                                       &absInfo);
            break;
          case EV_REP:
            // TODO
            break;
          default:
            libevdev_enable_event_code(device.output_dev, eventType, evcode,
                                       nullptr);
            break;
          }
          lua_pop(EvTranslator::L, 1);
        }

        lua_pop(EvTranslator::L, 1);
      }
      break;
    case LUA_TSTRING:
      if (strcmp(lua_tostring(EvTranslator::L, -1), "input")) {
        std::cout << "advertise string invalid" << std::endl;
        EvTranslator::run = false;
        return;
      }
      for (int type = 1; type < EV_CNT; type++) {
        if (libevdev_has_event_type(EvTranslator::input_dev, type)) {
          for (int code = 0; code <= libevdev_event_type_get_max(type);
               code++) {
            if (libevdev_has_event_code(EvTranslator::input_dev, type, code)) {
              switch (type) {
              case EV_ABS:
                libevdev_enable_event_code(
                    device.output_dev, type, code,
                    libevdev_get_abs_info(EvTranslator::input_dev, code));
                break;
              case EV_REP:
                // TODO
                // int repeat = libevdev_get_repeat(this->input_dev,)
                // libevdev_enable_event_code(device.output_dev, type, code,
                //                            nullptr);
                break;
              default:
                libevdev_enable_event_code(device.output_dev, type, code,
                                           nullptr);
                break;
              }
            }
          }
        }
      }
      for (int property = 0; property < INPUT_PROP_CNT; property++) {
        if (libevdev_has_property(EvTranslator::input_dev, property)) {
          libevdev_enable_property(device.output_dev, property);
        }
      }
      break;
    default:
      std::cout << "advertise is not set or is invalid" << std::endl;
      EvTranslator::run = false;
      return;
      break;
    }
    lua_pop(EvTranslator::L, 1);

    // Optional parameters
    if (lua_getfield(EvTranslator::L, -1, "deviceBus") != LUA_TNUMBER) {
      libevdev_set_id_bustype(device.output_dev, 0);
    } else {
      libevdev_set_id_bustype(device.output_dev,
                              lua_tonumber(EvTranslator::L, -1));
    }
    lua_pop(EvTranslator::L, 1);

    if (lua_getfield(EvTranslator::L, -1, "deviceVendor") != LUA_TNUMBER) {
      libevdev_set_id_vendor(device.output_dev, 0);
    } else {
      libevdev_set_id_vendor(device.output_dev,
                             lua_tonumber(EvTranslator::L, -1));
    }
    lua_pop(EvTranslator::L, 1);

    if (lua_getfield(EvTranslator::L, -1, "deviceProduct") != LUA_TNUMBER) {
      libevdev_set_id_product(device.output_dev, 0);
    } else {
      libevdev_set_id_product(device.output_dev,
                              lua_tonumber(EvTranslator::L, -1));
    }
    lua_pop(EvTranslator::L, 1);

    if (lua_getfield(EvTranslator::L, -1, "deviceVersion") != LUA_TNUMBER) {
      libevdev_set_id_version(device.output_dev, 0);
    } else {
      libevdev_set_id_version(device.output_dev,
                              lua_tonumber(EvTranslator::L, -1));
    }
    lua_pop(EvTranslator::L, 1);

    if (lua_getfield(EvTranslator::L, -1, "properties") == LUA_TTABLE) {
      lua_pushnil(EvTranslator::L);
      while (lua_next(EvTranslator::L, -2) != 0) {
        if (!lua_isstring(L, -1)) {
          std::cout << "Property must be string" << std::endl;
          EvTranslator::run = false;
          return;
        }

        auto property = libevdev_property_from_name(lua_tostring(L, -1));
        if (property < 0) {
          std::cout << "Invalid propety name" << std::endl;
          EvTranslator::run = false;
          return;
        }
        libevdev_enable_property(device.output_dev, property);

        lua_pop(EvTranslator::L, 1);
      }
    }
    lua_pop(EvTranslator::L, 1);

    // lua_pop(EvTranslator::L, 1);
    // Create device
    libevdev_uinput_create_from_device(device.output_dev,
                                       LIBEVDEV_UINPUT_OPEN_MANAGED,
                                       &device.output_dev_uinput);
    lua_pop(EvTranslator::L, 1);
    EvTranslator::devices.push_back(device);
  }
}

struct eventForDevice {
  int device;
  int type;
  int code;
  int value;
};

int getEventsFromLua(lua_State *L,
                     std::vector<struct eventForDevice> &outEvents);

int sendEventToLua(lua_State *L, struct input_event ev,
                   std::vector<struct eventForDevice> &outEvents) {
  lua_getglobal(L, "HandleEvent");
  if (!lua_isfunction(L, -1)) {
    std::cout << "HandleEvent is not a function" << std::endl;
    return -1;
  }
  lua_createtable(L, 0, 0);
  lua_pushstring(L, libevdev_event_type_get_name(ev.type));
  lua_setfield(L, -2, "type");
  lua_pushstring(L, libevdev_event_code_get_name(ev.type, ev.code));
  lua_setfield(L, -2, "code");
  lua_pushinteger(L, ev.value);
  lua_setfield(L, -2, "value");
  // TODO
  //  lua_pushnumber(L, ev.time);
  //  lua_setfield(L, -2, "time");
  if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
    std::cout << "Error while calling function HandleEvent:" << std::endl;
    std::cout << lua_error(L) << std::endl;
    return -1;
  }
  getEventsFromLua(L, outEvents);
  return 0;
}

int getEventsFromLua(lua_State *L,
                     std::vector<struct eventForDevice> &outEvents) {

  if (lua_isnil(L, -1)) {
    return 0;
  } else if (!lua_istable(L, -1)) {
    std::cout << "Function HandleEvent returned invalid value" << std::endl;
  }

  lua_pushnil(L);
  while (lua_next(L, -2)) {
    if (!lua_istable(L, -1)) {
      std::cout << "Function HandleEvent returned invalid value" << std::endl;
      return -1;
    }

    struct eventForDevice oev = {0, 0, 0, 0};
    if (lua_getfield(L, -1, "device") != LUA_TNUMBER) {
      return -1;
      std::cout << "Function HandleEvent returned invalid device value"
                << std::endl;
    } else {
      oev.device = lua_tointeger(L, -1);
    }
    lua_pop(L, 1);

    if (lua_getfield(L, -1, "type") != LUA_TSTRING) {
      return -1;
      std::cout << "Function HandleEvent returned invalid type value"
                << std::endl;
    } else {
      oev.type = libevdev_event_type_from_name(lua_tostring(L, -1));
      if (oev.type == -1) {
        std::cout << "Function HandleEvent returned invalid type value"
                  << std::endl;
        return -1;
      }
    }
    lua_pop(L, 1);

    if (lua_getfield(L, -1, "code") != LUA_TSTRING) {
      return -1;
      std::cout << "Function HandleEvent returned invalid code value"
                << std::endl;
    } else {
      oev.code = libevdev_event_code_from_name(oev.type, lua_tostring(L, -1));
      if (oev.code == -1) {
        std::cout << "Function HandleEvent returned invalid code value"
                  << std::endl;
        return -1;
      }
    }
    lua_pop(L, 1);

    if (lua_getfield(L, -1, "value") != LUA_TNUMBER) {
      return -1;
      std::cout << "Function HandleEvent returned invalid value" << std::endl;
    } else {
      oev.value = lua_tointeger(L, -1);
    }
    lua_pop(L, 1);

    lua_pop(L, 1);
    outEvents.push_back(oev);
  }

  lua_pop(L, 1);
  return 0;
}

void EvTranslator::eventLoop() {
  int rc;
  auto lastTime = std::chrono::high_resolution_clock::now();
  std::vector<input_event> events;
  std::vector<struct eventForDevice> outEvents;

  while (EvTranslator::run) {
    struct input_event ev;
    if (libevdev_has_event_pending(input_dev)) {
      rc = libevdev_next_event(
          input_dev, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_BLOCKING,
          &ev);
      if (rc == LIBEVDEV_READ_STATUS_SYNC) {
        while (rc == LIBEVDEV_READ_STATUS_SYNC) {
          rc = libevdev_next_event(input_dev, LIBEVDEV_READ_FLAG_SYNC, &ev);
        }
      } else if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
        // handle events
        if (ev.type != EV_SYN) {
          events.push_back(ev);
        } else {
          events.push_back(ev);
          for (auto event : events) {
            if (sendEventToLua(EvTranslator::L, event, outEvents))
              return;
          }
          events.clear();
        }
      }
    }

    auto now = std::chrono::high_resolution_clock::now();
    double delta =
        std::chrono::duration<double, std::milli>(now - lastTime).count();
    if (delta >= 10) {
      lua_getglobal(EvTranslator::L, "Update");
      if (lua_isfunction(EvTranslator::L, -1)) {
        lastTime = now;
        lua_pushnumber(EvTranslator::L, delta);
        if (lua_pcall(EvTranslator::L, 1, 1, 0) != LUA_OK) {
          std::cout << "Error while calling function Update" << std::endl;
          std::cout << lua_error(EvTranslator::L) << std::endl;
        }
        getEventsFromLua(L, outEvents);
      } else {
        lua_pop(L, -1);
      }
    }

    if (outEvents.size()) {
      for (auto devev : outEvents) {
        if (devev.device < 0 || devev.device >= EvTranslator::devices.size()) {
          std::cout << "Device id out of bounds" << std::endl;
          return;
        }
        libevdev_uinput_write_event(
            EvTranslator::devices[devev.device].output_dev_uinput, devev.type,
            devev.code, devev.value);
      }
      outEvents.clear();
    }

    std::this_thread::sleep_for(std::chrono::microseconds(500));
  }
}
