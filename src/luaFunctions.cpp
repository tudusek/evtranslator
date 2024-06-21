#include "../include/mapper.hpp"
#include <libevdev/libevdev.h>
#include <linux/input.h>
#include <lua.hpp>

void EvTranslator::addFunctionsToLua() {
  lua_createtable(L, 0, 0);
  lua_pushcfunction(L, EvTranslator::LuaFunctions::getABSInfo);
  lua_setfield(L, -2, "getABSInfo");
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
