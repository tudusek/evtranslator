--
-- this configuration turns gamepad to keyboard and mouse
--
print("")
print("////Gamepad configuration by Tudusek////")
print("")

function ScriptPath()
   local str = debug.getinfo(2, "S").source:sub(2)
   return str:match("(.*/)")
end

-- to look for scripts from same directory as this one 
package.path = package.path..";"..ScriptPath().."?.lua"

Utils = require("utils")

Devices = {
  {
    deviceName = "Gamepad",
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

Utils.axisDeadzones = {
  ABS_X = 16384,
  ABS_Y = 16384,
  ABS_HAT0X = 0,
  ABS_HAT0Y = 0,
  ABS_RX = 256,
  ABS_RY = 256
}

local outEvQueue = {}
local flushQueue = false

function TableConcat(t1, t2)
  for i = 1, #t2 do
    t1[#t1 + 1] = t2[i]
  end
  return t1
end

function Update(deltaTime)
  -- print(deltaTime)
  local queue = {}
  TableConcat(queue, Utils.AxisToRel(0, "ABS_RX", "REL_X", -10, 10, Utils.axisLastStates["ABS_RX"]))
  TableConcat(queue, Utils.AxisToRel(0, "ABS_RY", "REL_Y", 10, -10, Utils.axisLastStates["ABS_RY"]))
  TableConcat(queue, Utils.AxisToRel(0, "ABS_HAT0X", "REL_X", -2, 2, Utils.axisLastStates["ABS_HAT0X"]))
  TableConcat(queue, Utils.AxisToRel(0, "ABS_HAT0Y", "REL_Y", -2, 2, Utils.axisLastStates["ABS_HAT0Y"]))
  if (#queue == 0) then
    return nil
  end
  return queue
end

function HandleEvent(event)
  if (flushQueue) then
    outEvQueue = {}
    flushQueue = false
  end

  if (type(event) == "nil") then
    flushQueue = true
    return outEvQueue
  end

  -- Ignore EV_MSC
  if (event.type == "EV_MSC") then
    return nil
  end

  if (event.type == "EV_KEY") then
    if (event.code == "BTN_EAST") then
      table.insert(outEvQueue, Utils.ConstructEvent(0, "EV_KEY", "KEY_E", event.value))
      table.insert(outEvQueue, Utils.ConstructSynEvent(0))
      return nil
    end
    if (event.code == "BTN_C") then
      table.insert(outEvQueue, Utils.ConstructEvent(0, "EV_KEY", "KEY_SPACE", event.value))
      table.insert(outEvQueue, Utils.ConstructSynEvent(0))
      return nil
    end
    if (event.code == "BTN_NORTH") then
      table.insert(outEvQueue, Utils.ConstructEvent(0, "EV_KEY", "KEY_Q", event.value))
      table.insert(outEvQueue, Utils.ConstructSynEvent(0))
      return nil
    end
    if (event.code == "BTN_SOUTH") then
      table.insert(outEvQueue, Utils.ConstructEvent(0, "EV_KEY", "KEY_F", event.value))
      table.insert(outEvQueue, Utils.ConstructSynEvent(0))
      return nil
    end
    if (event.code == "BTN_TL") then
      table.insert(outEvQueue, Utils.ConstructEvent(0, "EV_KEY", "BTN_RIGHT", event.value))
      table.insert(outEvQueue, Utils.ConstructSynEvent(0))
      return nil
    end
    if (event.code == "BTN_TR") then
      table.insert(outEvQueue, Utils.ConstructEvent(0, "EV_KEY", "BTN_LEFT", event.value))
      table.insert(outEvQueue, Utils.ConstructSynEvent(0))
      return nil
    end
    if (event.code == "BTN_SELECT") then
      table.insert(outEvQueue, Utils.ConstructEvent(0, "EV_KEY", "KEY_LEFTSHIFT", event.value))
      table.insert(outEvQueue, Utils.ConstructSynEvent(0))
      return nil
    end
    if (event.code == "BTN_START") then
      table.insert(outEvQueue, Utils.ConstructEvent(0, "EV_KEY", "KEY_LEFTCTRL", event.value))
      table.insert(outEvQueue, Utils.ConstructSynEvent(0))
      return nil
    end
    if (event.code == "BTN_TR2") then
      table.insert(outEvQueue, Utils.ConstructEvent(0, "EV_KEY", "KEY_ESC", event.value))
      table.insert(outEvQueue, Utils.ConstructSynEvent(0))
      return nil
    end
    if (event.code == "BTN_WEST") then
      if (event.value ~= 0) then
        table.insert(outEvQueue, Utils.ConstructEvent(0, "EV_REL", "REL_WHEEL", 1))
        table.insert(outEvQueue, Utils.ConstructSynEvent(0))
      end
      return nil
    end
    if (event.code == "BTN_Z") then
      if (event.value ~= 0) then
        table.insert(outEvQueue, Utils.ConstructEvent(0, "EV_REL", "REL_WHEEL", -1))
        table.insert(outEvQueue, Utils.ConstructSynEvent(0))
      end
      return nil
    end
  end
  if (event.type == "EV_ABS") then
    if (event.code == "ABS_X") then
      TableConcat(outEvQueue, Utils.AxisToButton(0, event.code, "KEY_A", "KEY_D", event.value))
      -- TableConcat(outEvQueue, Utils.AxisToButton(0, event.code, "KEY_LEFT", "KEY_RIGHT", event.value))
      return nil
    end
    if (event.code == "ABS_Y") then
      TableConcat(outEvQueue, Utils.AxisToButton(0, event.code, "KEY_S", "KEY_W", event.value))
      -- TableConcat(outEvQueue, Utils.AxisToButton(0, event.code, "KEY_DOWN", "KEY_UP", event.value))
      return nil
    end
    if (event.code == "ABS_RX") then
      Utils.axisLastStates[event.code] = event.value
      return nil
    end
    if (event.code == "ABS_RY") then
      Utils.axisLastStates[event.code] = event.value
      return nil
    end
    if (event.code == "ABS_HAT0X") then
      Utils.axisLastStates[event.code] = event.value
      return nil
    end
    if (event.code == "ABS_HAT0Y") then
      Utils.axisLastStates[event.code] = event.value
      return nil
    end
  end
  return nil
end
