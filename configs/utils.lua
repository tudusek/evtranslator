local utils = {}

utils.absInfo = {}
utils.axisDeadzones = {}
utils.axisLastStates = {}


function utils.ConstructEvent(device, evType, evCode, evValue)
  return { device = device, type = evType, code = evCode, value = evValue }
end

function utils.ConstructSynEvent(device)
  return utils.ConstructEvent(device, "EV_SYN", "SYN_REPORT", 0)
end

function utils.axisCenter(axisCode)
  if (utils.absInfo[axisCode] == nil) then
    utils.absInfo[axisCode] = EvTranslator.getABSInfo(axisCode)
  end
  return math.ceil((utils.absInfo[axisCode].minimum + utils.absInfo[axisCode].maximum) / 2)
end

function utils.AxisToButton(device, axisCode, minButtonCode, maxButtonCode, axisValue)
  local events = {}

  if (utils.axisDeadzones[axisCode] == nil) then
    utils.axisDeadzones[axisCode] = 0
  end

  if (utils.axisLastStates[axisCode] ~= nil) then
    if (axisValue + utils.axisDeadzones[axisCode] < utils.axisCenter(axisCode)) then
      if (utils.axisLastStates[axisCode] - utils.axisDeadzones[axisCode] > utils.axisCenter(axisCode)) then
        table.insert(events, utils.ConstructEvent(device, "EV_KEY", maxButtonCode, 0))
        table.insert(events, utils.ConstructSynEvent(device))
      end
      table.insert(events, utils.ConstructEvent(device, "EV_KEY", minButtonCode, 1))
      table.insert(events, utils.ConstructSynEvent(device))
    elseif (axisValue - utils.axisDeadzones[axisCode] > utils.axisCenter(axisCode)) then
      if (utils.axisLastStates[axisCode] + utils.axisDeadzones[axisCode] < utils.axisCenter(axisCode)) then
        table.insert(events, utils.ConstructEvent(device, "EV_KEY", minButtonCode, 0))
        table.insert(events, utils.ConstructSynEvent(device))
      end
      table.insert(events, utils.ConstructEvent(device, "EV_KEY", maxButtonCode, 1))
      table.insert(events, utils.ConstructSynEvent(device))
    else
      if (utils.axisLastStates[axisCode] + utils.axisDeadzones[axisCode] < utils.axisCenter(axisCode)) then
        table.insert(events, utils.ConstructEvent(device, "EV_KEY", minButtonCode, 0))
        table.insert(events, utils.ConstructSynEvent(device))
      end
      if (utils.axisLastStates[axisCode] - utils.axisDeadzones[axisCode] > utils.axisCenter(axisCode)) then
        table.insert(events, utils.ConstructEvent(device, "EV_KEY", maxButtonCode, 0))
        table.insert(events, utils.ConstructSynEvent(device))
      end
    end
  end
  utils.axisLastStates[axisCode] = axisValue
  return events
end

function utils.Map(x, in_min, in_max, out_min, out_max)
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
end

function utils.AxisToRel(device, axisCode, relAxisCode, relMin, relMax, axisValue)
  local events = {}

  if (utils.axisDeadzones[axisCode] == nil) then
    utils.axisDeadzones[axisCode] = 0
  end
  if (utils.axisLastStates[axisCode] ~= nil) then
    if (axisValue + utils.axisDeadzones[axisCode] < utils.axisCenter(axisCode) or
          axisValue - utils.axisDeadzones[axisCode] > utils.axisCenter(axisCode)) then
      local outv = math.floor(
        utils.Map(axisValue,
          utils.absInfo[axisCode].minimum,
          utils.absInfo[axisCode].maximum,
          relMin,
          relMax))
      -- print(outv)
      table.insert(events, utils.ConstructEvent(device, "EV_REL", relAxisCode, outv))
      table.insert(events, utils.ConstructSynEvent(device))
    end
  end
  utils.axisLastStates[axisCode] = axisValue
  return events
end

return utils
