#include "../include/mapper.hpp"
#include <cstring>
#include <iostream>

EvTranslator::OutputDevice::OutputDevice() {
  this->output_dev = libevdev_new();
  this->output_dev_uinput = nullptr;

  if (!this->output_dev) {
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
    std::cout << "deviceName is not specified or is not a string" << std::endl;
    EvTranslator::run = false;
    return;
  } else {
    libevdev_set_name(this->output_dev, lua_tostring(EvTranslator::L, -1));
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
          libevdev_enable_event_code(this->output_dev, eventType, evcode,
                                     &absInfo);
          break;
        case EV_REP:
          // TODO
          break;
        default:
          libevdev_enable_event_code(this->output_dev, eventType, evcode,
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
        for (int code = 0; code <= libevdev_event_type_get_max(type); code++) {
          if (libevdev_has_event_code(EvTranslator::input_dev, type, code)) {
            switch (type) {
            case EV_ABS:
              libevdev_enable_event_code(
                  this->output_dev, type, code,
                  libevdev_get_abs_info(EvTranslator::input_dev, code));
              break;
            case EV_REP:
              // TODO
              // int repeat = libevdev_get_repeat(this->input_dev,)
              // libevdev_enable_event_code(device.output_dev, type, code,
              //                            nullptr);
              break;
            default:
              libevdev_enable_event_code(this->output_dev, type, code, nullptr);
              break;
            }
          }
        }
      }
    }
    for (int property = 0; property < INPUT_PROP_CNT; property++) {
      if (libevdev_has_property(EvTranslator::input_dev, property)) {
        libevdev_enable_property(this->output_dev, property);
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
    libevdev_set_id_bustype(this->output_dev, 0);
  } else {
    libevdev_set_id_bustype(this->output_dev,
                            lua_tonumber(EvTranslator::L, -1));
  }
  lua_pop(EvTranslator::L, 1);

  if (lua_getfield(EvTranslator::L, -1, "deviceVendor") != LUA_TNUMBER) {
    libevdev_set_id_vendor(this->output_dev, 0);
  } else {
    libevdev_set_id_vendor(this->output_dev, lua_tonumber(EvTranslator::L, -1));
  }
  lua_pop(EvTranslator::L, 1);

  if (lua_getfield(EvTranslator::L, -1, "deviceProduct") != LUA_TNUMBER) {
    libevdev_set_id_product(this->output_dev, 0);
  } else {
    libevdev_set_id_product(this->output_dev,
                            lua_tonumber(EvTranslator::L, -1));
  }
  lua_pop(EvTranslator::L, 1);

  if (lua_getfield(EvTranslator::L, -1, "deviceVersion") != LUA_TNUMBER) {
    libevdev_set_id_version(this->output_dev, 0);
  } else {
    libevdev_set_id_version(this->output_dev,
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
      libevdev_enable_property(this->output_dev, property);

      lua_pop(EvTranslator::L, 1);
    }
  }
  lua_pop(EvTranslator::L, 1);

  // lua_pop(EvTranslator::L, 1);
  // Create device
  if (libevdev_uinput_create_from_device(this->output_dev,
                                         LIBEVDEV_UINPUT_OPEN_MANAGED,
                                         &this->output_dev_uinput) < 0) {
    std::cout << "Unable to create uinput device" << std::endl;
    EvTranslator::run = false;
  }
}

EvTranslator::OutputDevice::~OutputDevice() {
  if (this->output_dev)
    libevdev_uinput_destroy(this->output_dev_uinput);
  if (this->output_dev_uinput)
    libevdev_free(this->output_dev);
}

int EvTranslator::OutputDevice::sendEvent(unsigned int type, unsigned int code,
                                          int value) {
  return libevdev_uinput_write_event(this->output_dev_uinput, type, code,
                                     value);
}
