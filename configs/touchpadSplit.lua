print("")
print("////Test configuration by Tudusek////")
print("")

Utils = require("configs.utils")

Devices = {
  {
    deviceName = "MouseP1",
    absInfo = {},
    advertise = {
      EV_REL = {
        "REL_X",
        "REL_Y"
      },
      EV_KEY = {
        "BTN_LEFT",
        "BTN_RIGHT",
        "BTN_MIDDLE"
      }
    }
  },
  {
    deviceName = "MouseP2",
    absInfo = {},
    advertise = {
      EV_REL = {
        "REL_X",
        "REL_Y"
      },
      EV_KEY = {
        "BTN_LEFT",
        "BTN_RIGHT",
        "BTN_MIDDLE"
      }
    }
  }
}
local absInfo = {
  ABS_MT_POSITION_X = { min = 0, max = 1297 },
  ABS_MT_POSITION_Y = { min = 0, max = 866 }
}
local mtLastState = {}

local activeSlot = 0

function HandleEvent(event)
  if (type(event) == "nil") then
    return nil
  end
  if (event.type == "EV_ABS") then
    if (event.code == "ABS_MT_TRACKING_ID") then
      print("EV_ABS:ABS_MT_TRACKING_ID:" .. event.value)
      if (event.value == -1) then
        mtLastState[activeSlot] = nil
      else
        if (mtLastState[activeSlot] == nil) then
          mtLastState[activeSlot] = {}
          mtLastState[activeSlot].ABS_MT_TRACKING_ID = event.value
        end
      end
      return nil
    end
    if (event.code == "ABS_MT_POSITION_X") then
      -- print("EV_ABS:ABS_MT_POSITION_X:" .. event.value)
      if (mtLastState[activeSlot] ~= nil) then
        if (mtLastState[activeSlot].ABS_MT_POSITION_X == nil) then
          mtLastState[activeSlot].ABS_MT_POSITION_X = event.value
          if (event.value < absInfo.ABS_MT_POSITION_X.max / 2) then
            mtLastState[activeSlot].device = 0
          else
            mtLastState[activeSlot].device = 1
          end
          return nil
        end
        if (mtLastState[activeSlot].device == 0 and
              event.value < absInfo.ABS_MT_POSITION_X.max / 2) then
          local res = {
            Utils.ConstructEvent(0, "EV_REL", "REL_X", event.value - mtLastState[activeSlot].ABS_MT_POSITION_X),
            Utils.ConstructSynEvent(0)
          }
          mtLastState[activeSlot].ABS_MT_POSITION_X = event.value
          return res
        end
        if (mtLastState[activeSlot].device == 1 and
              event.value >= absInfo.ABS_MT_POSITION_X.max / 2) then
          local res = {
            Utils.ConstructEvent(1, "EV_REL", "REL_X", event.value - mtLastState[activeSlot].ABS_MT_POSITION_X),
            Utils.ConstructSynEvent(1)
          }
          mtLastState[activeSlot].ABS_MT_POSITION_X = event.value
          return res
        end
        return nil
      end
      return nil
    end
    if (event.code == "ABS_MT_POSITION_Y") then
      if (mtLastState[activeSlot] ~= nil) then
        if (mtLastState[activeSlot].ABS_MT_POSITION_Y == nil) then
          mtLastState[activeSlot].ABS_MT_POSITION_Y = event.value
          return nil
        else
          local res = {
            Utils.ConstructEvent(mtLastState[activeSlot].device, "EV_REL", "REL_Y", event.value - mtLastState[activeSlot].ABS_MT_POSITION_Y),
            Utils.ConstructSynEvent(mtLastState[activeSlot].device)
          }
          mtLastState[activeSlot].ABS_MT_POSITION_Y = event.value
          return res
        end
      end
      return nil
    end
    if (event.code == "ABS_MT_SLOT") then
      print("EV_ABS:ABS_MT_SLOT:" .. event.value)
      activeSlot = event.value
      return nil
    end
  end
  return nil;
end
