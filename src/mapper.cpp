#include "../include/mapper.hpp"
#include <cerrno>
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
std::vector< std::unique_ptr<EvTranslator::OutputDevice>> EvTranslator::devices;
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
}

void EvTranslator::setupInputDev() {
  std::cout << "Openning " << in_device_path << std::endl;
  int input_dev_fd = open(in_device_path.c_str(), O_RDONLY);
  if (input_dev_fd < 0) {
    std::cout << "Failed to open input device" << std::endl;
    EvTranslator::run = false;
    return;
  }

  int error = libevdev_new_from_fd(input_dev_fd, &input_dev);
  if (error < 0) {
    std::cout << "Failed to init input device: " << error << std::endl;
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
    EvTranslator::devices.emplace_back(new EvTranslator::OutputDevice());
    lua_pop(EvTranslator::L, 1);
  }
}

struct eventForDevice {
  int device;
  int type;
  int code;
  int value;
};

int sendEventToLua(lua_State *L, struct input_event ev) {
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
  return 0;
}

void EvTranslator::eventLoop() {
  int rc;
  auto lastTime = std::chrono::high_resolution_clock::now();
  std::vector<input_event> events;

  while (EvTranslator::run) {
    struct input_event ev;
    int isEventPending = libevdev_has_event_pending(input_dev);
    if (isEventPending > 0) {
      rc = libevdev_next_event(
          input_dev, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_BLOCKING,
          &ev);
      if (rc == LIBEVDEV_READ_STATUS_SYNC) {
        while (rc == LIBEVDEV_READ_STATUS_SYNC) {
          rc = libevdev_next_event(input_dev, LIBEVDEV_READ_FLAG_SYNC, &ev);
        }
      } else if (rc == LIBEVDEV_READ_STATUS_SUCCESS || rc == -EAGAIN) {
        // handle events
        sendEventToLua(EvTranslator::L, ev);

      } else {
        std::cout << "Error while trying to check for events" << std::endl;
        EvTranslator::run = false;
      }
    } else if (isEventPending < 0) {
      std::cout << "Error while trying to check for events" << std::endl;
      EvTranslator::run = false;
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
      } else {
        lua_pop(L, -1);
      }
    }

    std::this_thread::sleep_for(std::chrono::microseconds(500));
  }
}
