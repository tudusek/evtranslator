--
-- this configuration turns keypad to separate keyboard
--
print("")
print("////Test configuration by Tudusek////")
print("")

Devices = {
  {
    deviceBus = 0,
    deviceVendor = 0,
    deviceProduct = 0,
    deviceVersion = 0,
    deviceName = "KeyboardP2",
    absInfo = {},
    advertise = {
      EV_KEY = {
        "KEY_W",
        "KEY_S",
        "KEY_A",
        "KEY_D",
        "KEY_SPACE",
        "KEY_Q",
        "KEY_E",
        "KEY_F",
        "KEY_ESC"
      }
    }
  },
  -- Fallback device
  {
    deviceName = "KeyboardP1",
    -- Will advertise all events and codes form input device
    advertise = "input"
  }
}

-- IN
-- event = {
--  type,
--  code,
--  value,
--  time
-- }

-- OUT
-- {{deviceID, type, code, value },... }
-- or nil

function CraftEvent(dev, t, c, val)
  return {
    { device = dev, type = t,        code = c,            value = val },
    { device = dev,   type = "EV_SYN", code = "SYN_REPORT", value = 0 }
  }
end

function HandleEvent(event)
  if (type(event) == "nil") then
    return nil
  end
  -- print("LUA:"..event.type..":"..event.code..":"..event.value)
  -- Ignore EV_MSC
  if (event.type == "EV_MSC") then
    return nil
  end
  if (event.type == "EV_KEY") then
    if (event.code == "KEY_KP8") then
      return CraftEvent(0, "EV_KEY", "KEY_W", event.value)
    end
    if (event.code == "KEY_KP5") then
      return CraftEvent(0, "EV_KEY", "KEY_S", event.value)
    end
    if (event.code == "KEY_KP4") then
      return CraftEvent(0, "EV_KEY", "KEY_A", event.value)
    end
    if (event.code == "KEY_KP6") then
      return CraftEvent(0, "EV_KEY", "KEY_D", event.value)
    end
    if (event.code == "KEY_KPENTER") then
      return CraftEvent(0, "EV_KEY", "KEY_SPACE", event.value)
    end
    if (event.code == "KEY_KP7") then
      return CraftEvent(0, "EV_KEY", "KEY_Q", event.value)
    end
    if (event.code == "KEY_KP9") then
      return CraftEvent(0, "EV_KEY", "KEY_E", event.value)
    end
    if (event.code == "KEY_KPPLUS") then
      return CraftEvent(0, "EV_KEY", "KEY_F", event.value)
    end
    if (event.code == "KEY_KPSLASH") then
      return CraftEvent(0, "EV_KEY", "KEY_ESC", event.value)
    end
  end
  -- Relay all unhandled events to fallback device
  return CraftEvent(1, event.type, event.code, event.value)
end
