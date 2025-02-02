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
-- EvTranslator.sendEvent(deviceID, type, code, value)

function CraftEvent(dev, t, c, val)
  return {
    { device = dev, type = t,        code = c,            value = val },
    { device = dev, type = "EV_SYN", code = "SYN_REPORT", value = 0 }
  }
end

local p1KeyPressed = false
local p2KeyPressed = false

function HandleEvent(event)
  if (event.type == "EV_SYN") then
    if (p1KeyPressed) then
      EvTranslator.sendEvent(1, event.type, event.code, event.value)
      p1KeyPressed = false
    end
    if (p2KeyPressed) then
      EvTranslator.sendEvent(0, event.type, event.code, event.value)
      p2KeyPressed = false
    end
  elseif (event.type == "EV_KEY") then
    if (event.code == "KEY_KP8") then
      EvTranslator.sendEvent(0, "EV_KEY", "KEY_W", event.value)
      p2KeyPressed = true
    elseif (event.code == "KEY_KP5") then
      EvTranslator.sendEvent(0, "EV_KEY", "KEY_S", event.value)
      p2KeyPressed = true
    elseif (event.code == "KEY_KP4") then
      EvTranslator.sendEvent(0, "EV_KEY", "KEY_A", event.value)
      p2KeyPressed = true
    elseif (event.code == "KEY_KP6") then
      EvTranslator.sendEvent(0, "EV_KEY", "KEY_D", event.value)
      p2KeyPressed = true
    elseif (event.code == "KEY_KPENTER") then
      EvTranslator.sendEvent(0, "EV_KEY", "KEY_SPACE", event.value)
      p2KeyPressed = true
    elseif (event.code == "KEY_KP7") then
      EvTranslator.sendEvent(0, "EV_KEY", "KEY_Q", event.value)
      p2KeyPressed = true
    elseif (event.code == "KEY_KP9") then
      EvTranslator.sendEvent(0, "EV_KEY", "KEY_E", event.value)
      p2KeyPressed = true
    elseif (event.code == "KEY_KPPLUS") then
      EvTranslator.sendEvent(0, "EV_KEY", "KEY_F", event.value)
      p2KeyPressed = true
    elseif (event.code == "KEY_KPSLASH") then
      EvTranslator.sendEvent(0, "EV_KEY", "KEY_ESC", event.value)
      p2KeyPressed = true
    else
      -- Relay all unhandled events to fallback device
      p1KeyPressed = true
      EvTranslator.sendEvent(1, event.type, event.code, event.value)
    end
  end
end
