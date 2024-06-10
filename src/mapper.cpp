#include "../include/mapper.hpp"
#include <chrono>
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

void EvTranslator::setupLua(std::string configPath) {
  this->L = luaL_newstate();
  luaL_openlibs(this->L);
  // luaopen_base(this->L);
  // luaopen_math(this->L);
  // luaopen_utf8(this->L);
  // luaopen_string(this->L);
  // luaopen_package(this->L);
  // luaopen_os(this->L);
  // luaopen_table(this->L);
  if (luaL_dofile(this->L, configPath.c_str()) != LUA_OK) {
    std::cout << "[C] Error reading script\n";
    luaL_error(this->L, "Error: %s\n", lua_tostring(this->L, -1));
    this->run = false;
    return;
  }

  if (lua_getglobal(this->L, "Devices") != LUA_TTABLE) {
    std::cout << "Configuration error" << std::endl;
    this->run = false;
    return;
  }
  auto len = lua_rawlen(this->L, -1);
  std::cout << "CFG count: " << len << std::endl;

  for (int i = 1; i <= len; i++) {
    EvTranslator::Device device;
    device.output_dev = nullptr;
    device.output_dev_uinput = nullptr;

    lua_pushnumber(this->L, i);
    if (lua_gettable(this->L, -2) != LUA_TTABLE) {
      std::cout << "value on index " << i << " is not a table" << std::endl;
      this->run = false;
      return;
    }

    if (lua_getfield(this->L, -1, "deviceName") != LUA_TSTRING) {
      std::cout << "deviceName is not specified or is not a string"
                << std::endl;
      this->run = false;
      return;
    } else {
      device.device_name = lua_tostring(this->L, -1);
      std::cout << device.device_name << ":" << std::endl;
    }
    lua_pop(L, 1);

    int advertise = lua_getfield(this->L, -1, "advertise");
    if (advertise == LUA_TTABLE) {
      lua_pushnil(this->L);
      while (lua_next(this->L, -2) != 0) {
        EvTranslator::Device::EventClass evc;
        if (!lua_isstring(this->L, -2)) {
          std::cout << "Invalid event type in advertise" << std::endl;
          this->run = false;
          return;
        }
        evc.eventType =
            libevdev_event_type_from_name(lua_tostring(this->L, -2));
        if (evc.eventType == -1) {
          std::cout << "Invalid event type in advertise" << std::endl;
          this->run = false;
          return;
        }

        std::cout << "\tEvent type: " << evc.eventType << std::endl;

        auto len2 = lua_rawlen(this->L, -1);
        for (int j = 1; j <= len2; j++) {
          lua_pushnumber(this->L, j);
          if (lua_gettable(this->L, -2) != LUA_TSTRING) {
            std::cout << "Invalid event code" << std::endl;
            this->run = false;
            return;
          }
          int evcode = libevdev_event_code_from_name(evc.eventType,
                                                     lua_tostring(this->L, -1));
          if (evcode == -1) {
            std::cout << "Invalid event code" << std::endl;
            this->run = false;
            return;
          }
          evc.eventCodes.push_back(evcode);
          std::cout << "\t\tEvent code: " << evcode << std::endl;
          lua_pop(this->L, 1);
        }
        device.advertisedCodes.push_back(evc);
        lua_pop(this->L, 1);
      }
    } else if (advertise == LUA_TSTRING) {
    } else {
      std::cout << "advertise is not set or is invalid" << std::endl;
      this->run = false;
      return;
    }
    lua_pop(this->L, 1);

    // Optional parameters
    if (lua_getfield(this->L, -1, "deviceBus") != LUA_TNUMBER) {
      device.device_bus = 0;
    } else {
      device.device_bus = lua_tonumber(this->L, -1);
    }
    lua_pop(this->L, 1);

    if (lua_getfield(this->L, -1, "deviceVendor") != LUA_TNUMBER) {
      device.device_vendor = 0;
    } else {
      device.device_vendor = lua_tonumber(this->L, -1);
    }
    lua_pop(this->L, 1);

    if (lua_getfield(this->L, -1, "deviceProduct") != LUA_TNUMBER) {
      device.device_product = 0;
    } else {
      device.device_product = lua_tonumber(this->L, -1);
    }
    lua_pop(this->L, 1);

    if (lua_getfield(this->L, -1, "deviceVersion") != LUA_TNUMBER) {
      device.device_version = 0;
    } else {
      device.device_version = lua_tonumber(this->L, -1);
    }
    lua_pop(this->L, 1);

    // Pop device table
    lua_pop(this->L, 1);
    this->devices.push_back(device);
  }
  // Pop Devices
  lua_pop(this->L, 1);
}

EvTranslator::EvTranslator(std::string inputDevPath, std::string configPath) {
  this->input_dev = nullptr;
  this->in_device_path = inputDevPath;
  setupLua(configPath);
  if (!this->run)
    return;
  setupInputDev();
  if (!this->run)
    return;
  setupOutputDev();
  if (!this->run)
    return;
  eventLoop();
}

EvTranslator::~EvTranslator() {
  if (this->L)
    lua_close(this->L);
  if (input_dev) {
    int fd = libevdev_get_fd(input_dev);
    libevdev_free(input_dev);
    close(fd);
  }
  for (auto &device : this->devices) {
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
    this->run = false;
    return;
  }
  int error = libevdev_new_from_fd(input_dev_fd, &input_dev);
  if (error < 0) {
    std::cout << "Failed to init input device" << std::endl;
    this->run = false;
    return;
  }
  std::cout << libevdev_get_name(input_dev) << std::endl;
  // check if the device has all evtypes and evcodes
  /*
  for (auto &device : this->devices) {
    for (auto &map : device.mappings) {
      if (!libevdev_has_event_code(this->input_dev, map.in.type, map.in.code)) {
        std::cout << "This device doesn't have "
                  << libevdev_event_type_get_name(map.in.type) << "("
                  << map.in.type << ")"
                  << " or "
                  << libevdev_event_code_get_name(map.in.code, map.in.code)
                  << "(" << map.in.code << ")" << std::endl;
        return -1;
      }
    }
  }
  */
}

void EvTranslator::setupOutputDev() {
  for (auto &device : this->devices) {
    device.output_dev = libevdev_new();
    device.output_dev_uinput = nullptr;
    if (!device.output_dev) {
      std::cout << "Failed to allocate memory for device" << std::endl;
      this->run = false;
      return;
    }
    // Setup device ids
    libevdev_set_name(device.output_dev, device.device_name.c_str());
    libevdev_set_id_bustype(device.output_dev, device.device_bus);
    libevdev_set_id_vendor(device.output_dev, device.device_vendor);
    libevdev_set_id_product(device.output_dev, device.device_product);
    libevdev_set_id_version(device.output_dev, device.device_version);
    if (device.advertisedCodes.size()) {
      for (auto evClass : device.advertisedCodes) {
        for (auto evcode : evClass.eventCodes) {
          switch (evClass.eventType) {
          case EV_ABS:
            // TODO
            break;
          case EV_REP:
            // TODO
            break;
          default:
            libevdev_enable_event_code(device.output_dev, evClass.eventType,
                                       evcode, nullptr);
            break;
          }
        }
      }
    } else {
      for (int type = 1; type < EV_CNT; type++) {
        if (libevdev_has_event_type(this->input_dev, type)) {
          for (int code = 0; code <= libevdev_event_type_get_max(type);
               code++) {
            if (libevdev_has_event_code(this->input_dev, type, code)) {
              switch (type) {
              case EV_ABS:
                libevdev_enable_event_code(
                    device.output_dev, type, code,
                    libevdev_get_abs_info(this->input_dev, code));
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
    }
    // Create device
    libevdev_uinput_create_from_device(device.output_dev,
                                       LIBEVDEV_UINPUT_OPEN_MANAGED,
                                       &device.output_dev_uinput);
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
                   std::vector<struct eventForDevice> &outEvents,
                   bool nil = false) {
  lua_getglobal(L, "HandleEvent");
  if (!lua_isfunction(L, -1)) {
    std::cout << "HandleEvent is not a function" << std::endl;
    return -1;
  }
  if (nil) {
    lua_pushnil(L);
  } else {
    lua_createtable(L, 0, 0);
    lua_pushstring(L, libevdev_event_type_get_name(ev.type));
    lua_setfield(L, -2, "type");
    lua_pushstring(L, libevdev_event_code_get_name(ev.type, ev.code));
    lua_setfield(L, -2, "code");
    lua_pushinteger(L, ev.value);
    lua_setfield(L, -2, "value");
    lua_pushinteger(L, ev.type);
    lua_setfield(L, -2, "time");
  }
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
  while (this->run) {
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
          for (auto event : events) {
            if (sendEventToLua(this->L, event, outEvents))
              return;
          }
          if (sendEventToLua(this->L, {0, 0, 0, 0}, outEvents, true))
            return;
          events.clear();
        }
      }
    }
    auto now = std::chrono::high_resolution_clock::now();
    double delta =
        std::chrono::duration<double, std::milli>(now - lastTime).count();
    if (delta >= 10) {
      lua_getglobal(this->L, "Update");
      if (lua_isfunction(this->L, -1)) {
        lastTime = now;
        lua_pushnumber(this->L, delta);
        if (lua_pcall(this->L, 1, 1, 0) != LUA_OK) {
          std::cout << "Error while calling function Update" << std::endl;
          std::cout << lua_error(this->L) << std::endl;
        }
        getEventsFromLua(L, outEvents);
      }
    }
    if (outEvents.size()) {
      for (auto devev : outEvents) {
        if (devev.device < 0 || devev.device >= this->devices.size()) {
          std::cout << "Device id out of bounds" << std::endl;
          return;
        }
        libevdev_uinput_write_event(
            this->devices[devev.device].output_dev_uinput, devev.type,
            devev.code, devev.value);
      }
      outEvents.clear();
    }
    std::this_thread::sleep_for(std::chrono::microseconds(500));
  }
}