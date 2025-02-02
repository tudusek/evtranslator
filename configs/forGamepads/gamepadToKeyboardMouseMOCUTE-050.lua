--
-- This configuration turns mocute-050 gamepad into keyboard and mouse.
--
--
print("")
print("////Gamepad configuration by Tudusek////")
print("")


Devices = {
  {
    deviceName = "Gamepad2",
    advertise = {
      EV_KEY = {
        "KEY_W",
        "KEY_S",
        "KEY_A",
        "KEY_D",
        "KEY_E",
        "KEY_Q",
        "KEY_F",
        "KEY_SPACE",
        "BTN_RIGHT",
        "BTN_LEFT",
        "KEY_LEFTSHIFT",
        "KEY_LEFTCTRL",
        "KEY_ESC"
      },
      EV_REL = {
        "REL_X",
        "REL_Y",
        "REL_WHEEL"
      }
    }
  }
}

local map = {
  EV_KEY = {
    BTN_TL2 = { evt = "EV_KEY", evc = "BTN_RIGHT" },
    BTN_TR2 = { evt = "EV_KEY", evc = "BTN_LEFT" },
    BTN_TL = { evt = "EV_REL", evc = "REL_WHEEL", val = 1 },
    BTN_TR = { evt = "EV_REL", evc = "REL_WHEEL", val = -1 },
    BTN_WEST = { evt = "EV_KEY", evc = "KEY_E" },
    BTN_SOUTH = { evt = "EV_KEY", evc = "KEY_SPACE" },
    BTN_NORTH = { evt = "EV_KEY", evc = "KEY_Q" },
    BTN_EAST = { evt = "EV_KEY", evc = "KEY_F" },
    BTN_START = { evt = "EV_KEY", evc = "KEY_ESC" },
    BTN_THUMBL = { evt = "EV_KEY", evc = "KEY_LEFTSHIFT" },
    BTN_THUMBR = { evt = "EV_KEY", evc = "KEY_LEFTCTRL" },
    BTN_SELECT = { evt = "EV_KEY", evc = "KEY_F3" },
  },
  EV_ABS = {
    ABS_X = { evt = "EV_KEY", evc = { "KEY_A", "KEY_D" }, dzone = 16 },
    ABS_Y = { evt = "EV_KEY", evc = { "KEY_W", "KEY_S" }, dzone = 16 },
    ABS_HAT0X = { evt = "EV_REL", evc = "REL_X", min = -2, max = 2 },
    ABS_HAT0Y = { evt = "EV_REL", evc = "REL_Y", min = -2, max = 2 },
    ABS_Z = { evt = "EV_REL", evc = "REL_X", dzone = 2, min = -10, max = 10 },
    ABS_RZ = { evt = "EV_REL", evc = "REL_Y", dzone = 2, min = -10, max = 10 },
  }
}

local toggleStates = {}
local axesLStates = {}
local axesABSInfo = {}

function ConstructEvent(device, evType, evCode, evValue)
  return { device = device, type = evType, code = evCode, value = evValue }
end

function ConstructSynEvent(device)
  return ConstructEvent(device, "EV_SYN", "SYN_REPORT", 0)
end

function AxisCenter(axCode)
  if (axesABSInfo[axCode] == nil) then
    axesABSInfo[axCode] = EvTranslator.getABSInfo(axCode)
  end
  return math.ceil((axesABSInfo[axCode].minimum + axesABSInfo[axCode].maximum) / 2)
end

function AxisToButton(device, axCode, axDZone, minBtnCode, maxBtnCode, axValue)
  local events = {}
  if (axesLStates[axCode] ~= nil) then
    if (axValue + axDZone < AxisCenter(axCode)) then
      if (axesLStates[axCode] - axDZone > AxisCenter(axCode)) then
        table.insert(events, ConstructEvent(device, "EV_KEY", maxBtnCode, 0))
        table.insert(events, ConstructSynEvent(device))
      end
      table.insert(events, ConstructEvent(device, "EV_KEY", minBtnCode, 1))
      table.insert(events, ConstructSynEvent(device))
    elseif (axValue - axDZone > AxisCenter(axCode)) then
      if (axesLStates[axCode] + axDZone < AxisCenter(axCode)) then
        table.insert(events, ConstructEvent(device, "EV_KEY", minBtnCode, 0))
        table.insert(events, ConstructSynEvent(device))
      end
      table.insert(events, ConstructEvent(device, "EV_KEY", maxBtnCode, 1))
      table.insert(events, ConstructSynEvent(device))
    else
      if (axesLStates[axCode] + axDZone < AxisCenter(axCode)) then
        table.insert(events, ConstructEvent(device, "EV_KEY", minBtnCode, 0))
        table.insert(events, ConstructSynEvent(device))
      end
      if (axesLStates[axCode] - axDZone > AxisCenter(axCode)) then
        table.insert(events, ConstructEvent(device, "EV_KEY", maxBtnCode, 0))
        table.insert(events, ConstructSynEvent(device))
      end
    end
  end
  return events
end

function AxisToRel(device, axCode, axDZone, relAxCode, relMin, relMax, axValue)
  local events = {}
  if (axDZone == nil) then
    axDZone = 0
  end

  if (axValue + axDZone < AxisCenter(axCode) or
        axValue - axDZone > AxisCenter(axCode)) then
    local outv = math.floor(
      (axValue - axesABSInfo[axCode].minimum)
      * (relMax - relMin)
      / (axesABSInfo[axCode].maximum - axesABSInfo[axCode].minimum)
      + relMin)
    table.insert(events, ConstructEvent(device, "EV_REL", relAxCode, outv))
    table.insert(events, ConstructSynEvent(device))
  end
  return events
end

function Update(deltaTime)
  for axCode, value in pairs(map["EV_ABS"]) do
    if (value.evt == "EV_REL") then
      if (axesLStates[axCode] ~= nil) then
        for index, event in ipairs(AxisToRel(0, axCode, value.dzone, value.evc, value.min, value.max, axesLStates[axCode])) do
          EvTranslator.sendEvent(event.device, event.type, event.code, event.value)
        end
      end
    end
  end
end

function HandleEvent(event)
  if (map[event.type] ~= nil and map[event.type][event.code] ~= nil) then
    if (event.type == "EV_KEY") then
      if (map[event.type][event.code].evt == "EV_KEY") then
        if (map[event.type][event.code].toggle) then
          if (event.value == 1 and (toggleStates[map[event.type][event.code].evc] == nil or toggleStates[map[event.type][event.code].evc] == false)) then
            EvTranslator.sendEvent(0, map[event.type][event.code].evt, map[event.type][event.code].evc, 1)
            EvTranslator.sendEvent(0, "EV_SYN", "SYN_REPORT", 0)
            toggleStates[map[event.type][event.code].evc] = true
          elseif (event.value == 1 and toggleStates[map[event.type][event.code].evc] == true) then
            EvTranslator.sendEvent(0, map[event.type][event.code].evt, map[event.type][event.code].evc, 0)
            EvTranslator.sendEvent(0, "EV_SYN", "SYN_REPORT", 0)
            toggleStates[map[event.type][event.code].evc] = false
          end
        else
          EvTranslator.sendEvent(0, map[event.type][event.code].evt, map[event.type][event.code].evc, event.value)
          EvTranslator.sendEvent(0, "EV_SYN", "SYN_REPORT", 0)
        end
      end
      if (map[event.type][event.code].evt == "EV_REL" and event.value == 1) then
        EvTranslator.sendEvent(0,
          map[event.type][event.code].evt,
          map[event.type][event.code].evc,
          map[event.type][event.code].val)
        EvTranslator.sendEvent(0, "EV_SYN", "SYN_REPORT", 0)
      end
    end
    if (event.type == "EV_ABS") then
      if (map[event.type][event.code].evt == "EV_KEY") then
        for index, axisEvent in ipairs(AxisToButton(
          0,
          event.code,
          map[event.type][event.code].dzone,
          map[event.type][event.code].evc[1],
          map[event.type][event.code].evc[2],
          event.value)) do
          EvTranslator.sendEvent(axisEvent.device, axisEvent.type, axisEvent.code, axisEvent.value)
        end
        axesLStates[event.code] = event.value
      end
      if (map[event.type][event.code].evt == "EV_REL") then
        --handled in Update
        axesLStates[event.code] = event.value
      end
    end
  end
end
