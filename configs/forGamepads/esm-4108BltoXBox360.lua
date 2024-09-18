--
-- This configuration turns gamepad into keyboard and mouse.
-- You probably want to change the keymap if you are not using EasySMX esm-4108 with blacklisted hid_nintendo.
--
print("")
print("////Gamepad configuration by Tudusek////")
print("")


Devices = {
  {
    deviceName = "ESM-4108 XBox 360",
    advertise = {
      EV_KEY = {
        "BTN_SOUTH",
        "BTN_EAST",
        "BTN_NORTH",
        "BTN_WEST",
        "BTN_TL",
        "BTN_TR",
        "BTN_SELECT",
        "BTN_START",
        "BTN_MODE",
        "BTN_THUMBL",
        "BTN_THUMBR"
      },
      EV_ABS = {
        "ABS_X",
        "ABS_Y",
        "ABS_Z",
        "ABS_RX",
        "ABS_RY",
        "ABS_RZ",
        "ABS_HAT0X",
        "ABS_HAT0Y"
      }
    },
    absInfo = {
      ABS_X = {
        value = 0,
        minimum = -32768,
        maximum = 32767,
        fuzz = 16,
        flat = 128,
        resolution = 0
      },
      ABS_Y = {
        value = 0,
        minimum = -32768,
        maximum = 32767,
        fuzz = 16,
        flat = 128,
        resolution = 0
      },
      ABS_Z = {
        value = 0,
        minimum = 0,
        maximum = 255,
        fuzz = 0,
        flat = 0,
        resolution = 0
      },
      ABS_RX = {
        value = 0,
        minimum = -32768,
        maximum = 32767,
        fuzz = 16,
        flat = 128,
        resolution = 0
      },
      ABS_RY = {
        value = 0,
        minimum = -32768,
        maximum = 32767,
        fuzz = 16,
        flat = 128,
        resolution = 0
      },
      ABS_RZ = {
        value = 0,
        minimum = 0,
        maximum = 255,
        fuzz = 0,
        flat = 0,
        resolution = 0
      },
      ABS_HAT0X = {
        value = 0,
        minimum = -1,
        maximum = 1,
        fuzz = 0,
        flat = 0,
        resolution = 0
      },
      ABS_HAT0Y = {
        value = 0,
        minimum = -1,
        maximum = 1,
        fuzz = 0,
        flat = 0,
        resolution = 0
      }
    }
  }
}


local map = {
  EV_KEY = {
    BTN_SOUTH = { evt = "EV_KEY", evc = "BTN_EAST" },
    BTN_EAST = { evt = "EV_KEY", evc = "BTN_WEST" },
    BTN_C = { evt = "EV_KEY", evc = "BTN_SOUTH" },
    BTN_NORTH = "pass",
    BTN_WEST = { evt = "EV_KEY", evc = "BTN_TL" },
    BTN_Z = { evt = "EV_KEY", evc = "BTN_TR" },
    BTN_TL = { evt = "EV_ABS", evc = "ABS_Z" },
    BTN_TR = { evt = "EV_ABS", evc = "ABS_RZ" },
    BTN_TL2 = { evt = "EV_KEY", evc = "BTN_SELECT" },
    BTN_TR2 = { evt = "EV_KEY", evc = "BTN_START" },
    BTN_SELECT = { evt = "EV_KEY", evc = "BTN_THUMBL" },
    BTN_START = { evt = "EV_KEY", evc = "BTN_THUMBR" },
    BTN_MODE = "pass",
    -- BTN_THUMBL = { evt = "EV_KEY", evc = "KEY_E" },
  },
  EV_ABS = {
    ABS_X = { evt = "EV_ABS", evc = "ABS_X", offset = -32768 },
    ABS_Y = { evt = "EV_ABS", evc = "ABS_Y", offset = -32767, invert = true },
    ABS_RX = { evt = "EV_ABS", evc = "ABS_RX", offset = -32768 },
    ABS_RY = { evt = "EV_ABS", evc = "ABS_RY", offset = -32767, invert = true },
    ABS_HAT0X = "pass",
    ABS_HAT0Y = "pass"
  }
}

function ConstructEvent(device, evType, evCode, evValue)
  return { device = device, type = evType, code = evCode, value = evValue }
end

function ConstructSynEvent(device)
  return ConstructEvent(device, "EV_SYN", "SYN_REPORT", 0)
end

function HandleEvent(event)
  if (event.type == "EV_SYN") then
    return { ConstructEvent(0, event.type, event.code, event.value) }
  end
  if (map[event.type] ~= nil and map[event.type][event.code] ~= nil) then
    if (type(map[event.type][event.code]) == "string" and map[event.type][event.code] == "pass") then
      return { ConstructEvent(0, event.type, event.code, event.value) }
    elseif (type(map[event.type][event.code]) == "table") then
      if (event.type == "EV_KEY") then
        if (map[event.type][event.code].evt == "EV_KEY") then
          return { ConstructEvent(0, map[event.type][event.code].evt, map[event.type][event.code].evc, event.value) }
        elseif (map[event.type][event.code].evt == "EV_ABS") then
          if (event.value == 0) then
            return { ConstructEvent(0,
              map[event.type][event.code].evt,
              map[event.type][event.code].evc,
              Devices[1].absInfo[map[event.type][event.code].evc].minimum) }
          elseif (event.value == 1) then
            return { ConstructEvent(0,
              map[event.type][event.code].evt,
              map[event.type][event.code].evc,
              Devices[1].absInfo[map[event.type][event.code].evc].maximum) }
          end
        end
      elseif (event.type == "EV_ABS") then
        local val = event.value;
        if (map[event.type][event.code].offset ~= nil) then
          val = val + map[event.type][event.code].offset
        end
        if (map[event.type][event.code].invert ~= nil and map[event.type][event.code].invert) then
          val = val * -1;
        end
        return { ConstructEvent(0,
          map[event.type][event.code].evt,
          map[event.type][event.code].evc,
          val) }
      end
    end
  end
end
