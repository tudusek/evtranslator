#include "../include/mapper.hpp"
#include <libevdev/libevdev.h>
#include <linux/input.h>
#include <lua.h>
#include <lua.hpp>

void EvTranslator::addFunctionsToLua() {
  lua_createtable(L, 0, 0);
  lua_pushcfunction(L, EvTranslator::LuaFunctions::getABSInfo);
  lua_setfield(L, -2, "getABSInfo");
  lua_pushcfunction(L, EvTranslator::LuaFunctions::hasProperty);
  lua_setfield(L, -2, "hasProperty");
  lua_pushcfunction(L, EvTranslator::LuaFunctions::hasEventType);
  lua_setfield(L, -2, "hasEventType");
  lua_pushcfunction(L, EvTranslator::LuaFunctions::hasEventCode);
  lua_setfield(L, -2, "hasEventCode");
  lua_pushcfunction(L, EvTranslator::LuaFunctions::sendEvent);
  lua_setfield(L, -2, "sendEvent");
  lua_setglobal(L, "EvTranslator");
}

int EvTranslator::LuaFunctions::getABSInfo(lua_State *L) {
  if (lua_gettop(L) != 1) {
    return luaL_error(L, "Invalid number of arguments");
  }
  if (!lua_isstring(L, -1)) {
    return luaL_error(L, "Argument must be string");
  }

  struct input_absinfo absinfo = *libevdev_get_abs_info(
      EvTranslator::input_dev,
      libevdev_event_code_from_name(EV_ABS, lua_tostring(L, -1)));
  lua_pop(L, 1);

  lua_createtable(L, 0, 0);
  lua_pushinteger(L, absinfo.value);
  lua_setfield(L, -2, "value");
  lua_pushinteger(L, absinfo.minimum);
  lua_setfield(L, -2, "minimum");
  lua_pushinteger(L, absinfo.maximum);
  lua_setfield(L, -2, "maximum");
  lua_pushinteger(L, absinfo.fuzz);
  lua_setfield(L, -2, "fuzz");
  lua_pushinteger(L, absinfo.flat);
  lua_setfield(L, -2, "flat");
  lua_pushinteger(L, absinfo.resolution);
  lua_setfield(L, -2, "resolution");
  return 1;
}

int EvTranslator::LuaFunctions::hasProperty(lua_State *L) {
  if (lua_gettop(L) != 1) {
    return luaL_error(L, "Invalid number of arguments");
  }

  if (!lua_isstring(L, -1)) {
    return luaL_error(L, "Argument must be string");
  }

  auto property = libevdev_property_from_name(lua_tostring(L, -1));
  lua_pop(L, 1);

  if (property < 0) {
    return luaL_error(L, "Invalid property name");
  }
  lua_pushboolean(L, libevdev_has_property(EvTranslator::input_dev, property));
  return 1;
}

int EvTranslator::LuaFunctions::hasEventType(lua_State *L) {
  if (lua_gettop(L) != 1) {
    return luaL_error(L, "Invalid number of arguments");
  }

  if (!lua_isstring(L, -1)) {
    return luaL_error(L, "Argument must be string");
  }

  auto type = libevdev_event_type_from_name(lua_tostring(L, -1));
  lua_pop(L, 1);

  if (type < 0) {
    return luaL_error(L, "Invalid type name");
  }
  lua_pushboolean(L, libevdev_has_event_type(EvTranslator::input_dev, type));
  return 1;
}

int EvTranslator::LuaFunctions::hasEventCode(lua_State *L) {
  if (lua_gettop(L) != 2) {
    return luaL_error(L, "Invalid number of arguments");
  }

  if (!lua_isstring(L, -1)) {
    return luaL_error(L, "Arguments must be string");
  }

  if (!lua_isstring(L, -2)) {
    return luaL_error(L, "Arguments must be string");
  }

  auto type = libevdev_event_type_from_name(lua_tostring(L, -2));
  if (type < 0) {
    return luaL_error(L, "Invalid type name");
  }
  auto code = libevdev_event_code_from_name(type, lua_tostring(L, -1));
  if (code < 0) {
    return luaL_error(L, "Invalid code name");
  }

  lua_pop(L, 2);

  lua_pushboolean(L,
                  libevdev_has_event_code(EvTranslator::input_dev, type, code));

  return 1;
}

int EvTranslator::LuaFunctions::sendEvent(lua_State *L) {
  if (lua_gettop(L) != 4) {
    return luaL_error(L, "Invalid number of arguments");
  }

  if (!lua_isinteger(L, -1)) {
    return luaL_error(L, "Arguments must be string");
  }

  if (!lua_isstring(L, -2)) {
    return luaL_error(L, "Arguments must be string");
  }

  if (!lua_isstring(L, -3)) {
    return luaL_error(L, "Arguments must be string");
  }

  if (!lua_isinteger(L, -4)) {
    return luaL_error(L, "Arguments must be integer");
  }
  int device = lua_tointeger(L, -4);
  auto type = libevdev_event_type_from_name(lua_tostring(L, -3));
  auto code = libevdev_event_code_from_name(type, lua_tostring(L, -2));
  int value = lua_tointeger(L, -1);

  lua_pop(L, 4);

  if (device < 0 || device >= EvTranslator::devices.size()) {
    return luaL_error(L, "Device id out of bounds");
  }

  if (type < 0) {
    return luaL_error(L, "Invalid type name");
  }

  if (code < 0) {
    return luaL_error(L, "Invalid code name");
  }

  EvTranslator::devices[device]->sendEvent(type, code, value);
  return 0;
}
