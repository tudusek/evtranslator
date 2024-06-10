# Evtranslator
Turn any input device to any input device(s) on Linux

## Usage
`evtranslator /dev/input/eventX /path/to/configuration.lua`
You need to have permissions to open */dev/uinput* and the event device you want to use

### Lua configuration
Before you start read this [article](https://www.baeldung.com/linux/mouse-events-input-event-interface) to get better understanding how Linux input events work.

Evtranslator uses for inicialization table Devices. It conains specification(s) of device(s).

`Devices = { {device1 }, ...  }`

Device specification might look like this:
```
Devices = {
  {
    deviceBus = 0, -- optional
    deviceVendor = 0, -- optional
    deviceProduct = 0, -- optional
    deviceVersion = 0, -- optional
    deviceName = "YourDeviceName",
    absInfo = {}, -- not done yet
    -- events that will be advertised on this device
    -- advertise = "input" will advertise all event types and codes from input device
    advertise = {
      EV_KEY = { -- event type
        "KEY_W", -- event code
        "KEY_S",
        "KEY_A",
        "KEY_D"
      }
    }
  }
}
```

Evtranslator calls to fucntions from Lua file:
- HandleEvent(event)
  - called for every event from input device
  - if event from input device is EV_SYN then event parameter is nil 
- Update(deltaTime) (optional)
  - called aproximatelly every 10 miliseconds

Both functions needs to return table of events or nil.
Example for sending key press to first device:

```
return {
  { device = 0, type = "EV_KEY", code = "KEY_W", value = 1 },
  { device = 0, type = "EV_SYN", code = "SYN_REPORT", value = 0 },
}
```

Also checkout configs directory for examples
