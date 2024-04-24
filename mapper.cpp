#include "mapper.hpp"
#include <cstdlib>
#include <iostream>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <ostream>
#include <string>

void evdevMapper::parseParameters(int argc, char *argv[]) {
  if (argc == 3) {
    this->in_device_path = argv[1];
    std::ifstream f(argv[2]);
    this->data = json::parse(f);
  } else {
    std::cout
        << "Usage: evdevmapper /path/to/device /path/to/configuration.json"
        << std::endl;
    exit(0);
  }
}
evdevMapper::evdevMapper(int argc, char *argv[]) {
  parseParameters(argc, argv);
  setupInputDev();
  setupOutputDev();
  eventLoop();
}
evdevMapper::~evdevMapper() {
  int fd = libevdev_get_fd(input_dev);
  libevdev_free(input_dev);
  close(fd);
  for (auto i : output_devs) {
    libevdev_free(i);
  }
  for (auto i : output_devs_uinput){
    libevdev_uinput_destroy(i);
  }
}
void evdevMapper::setupInputDev() {
  int input_dev_fd = open(in_device_path.c_str(), O_RDONLY);
  if (input_dev_fd < 0) {
    std::cout << "Failed to open input device" << std::endl;
    exit(0);
  }
  int error = libevdev_new_from_fd(input_dev_fd, &input_dev);
  if (error < 0) {
    std::cout << "Failed to init input device" << std::endl;
    exit(0);
  }
  std::cout << libevdev_get_name(input_dev) << std::endl;
  // check if the device has all evtypes and evcodes
  if (this->data.is_array()) {
    for (auto &[confignum, config] : this->data.items()) {
      std::cout << confignum << std::endl;
      for (auto &[evtype, evcodes] : config["mappings"].items()) {
        std::cout << "  " << evtype << std::endl;
        __u16 evt = libevdev_event_type_from_name(evtype.c_str());
        for (auto &[evcode, evprops] : evcodes.items()) {
          std::cout << "    " << evcode << std::endl;
          __u16 evc = libevdev_event_code_from_name(evt, evcode.c_str());
          if (!libevdev_has_event_code(this->input_dev, evt, evc)) {
            std::cout << "This device doesn't have " << evtype << "(" << evt
                      << ")"
                      << " or " << evcode << "(" << evc << ")" << std::endl;
            exit(0);
          }
        }
      }
    }
  } else {
    std::cout << "Incompatible configuration" << std::endl;
    exit(0);
  }
}
void evdevMapper::setupOutputDev() {
  for (auto &[confignum, config] : this->data.items()) {
    struct libevdev *output_dev = libevdev_new();
    struct libevdev_uinput *output_dev_uinput = nullptr;
    if (config["device-name"].is_null())
      libevdev_set_name(output_dev, "evdev-mapper");
    else
      libevdev_set_name(output_dev,
                        config["device-name"].get<std::string>().c_str());
    if (!config["mappings"].is_null()) {
      for (auto &[evtype, evcodes] : config["mappings"].items()) {
        __u16 evt = libevdev_event_type_from_name(evtype.c_str());
        for (auto &[evcode, evprops] : evcodes.items()) {
          if (!evprops["event-type"].is_null() &&
              !evprops["event-code"].is_null()) {
            __u16 oevt = libevdev_event_type_from_name(
                evprops["event-type"].get<std::string>().c_str());
            __u16 oevc = libevdev_event_code_from_name(
                oevt, evprops["event-code"].get<std::string>().c_str());
            switch (evt) {
            case EV_KEY:
              libevdev_enable_event_code(output_dev, oevt, oevc, nullptr);
              break;
            }
          }
        }
      }
    }
    libevdev_uinput_create_from_device(output_dev, LIBEVDEV_UINPUT_OPEN_MANAGED,
                                       &output_dev_uinput);
    this->output_devs.push_back(output_dev);
    this->output_devs_uinput.push_back(output_dev_uinput);
  }
}
void evdevMapper::eventLoop() {
  int rc;
  do {
    struct input_event ev;
    rc = libevdev_next_event(
        this->input_dev,
        LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_BLOCKING, &ev);
    if (rc == LIBEVDEV_READ_STATUS_SYNC) {
      printf("::::::::::::::::::::: dropped ::::::::::::::::::::::\n");
      while (rc == LIBEVDEV_READ_STATUS_SYNC) {
        rc = libevdev_next_event(this->input_dev, LIBEVDEV_READ_FLAG_SYNC, &ev);
      }
      printf("::::::::::::::::::::: re-synced ::::::::::::::::::::::\n");
    } else if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
      // handle events
      std::cout << "Event:" << std::endl
                << "  " << ev.type << " " << ev.code << " " << ev.value
                << std::endl;
      for (auto &[confignum, config] : this->data.items()) {
        int cfgnum = std::stoi(confignum);
        for (auto &[evtype, evcodes] : config["mappings"].items()) {
          __u16 evt = libevdev_event_type_from_name(evtype.c_str());
          for (auto &[evcode, evprops] : evcodes.items()) {
            __u16 evc = libevdev_event_code_from_name(evt, evcode.c_str());
            if (ev.type != EV_SYN && ev.type == evt && ev.code == evc) {
              if (!evprops["event-type"].is_null() &&
                  !evprops["event-code"].is_null()) {
                __u16 oevt = libevdev_event_type_from_name(
                    evprops["event-type"].get<std::string>().c_str());
                __u16 oevc = libevdev_event_code_from_name(
                    oevt, evprops["event-code"].get<std::string>().c_str());
                std::cout << "  Found cfg ("<< cfgnum <<"):" << std::endl
                          << "    " << ev.type << " " << ev.code << " " << oevt
                          << " " << oevc << std::endl;
                libevdev_uinput_write_event(this->output_devs_uinput[cfgnum], oevt, oevc,
                                            ev.value);
                libevdev_uinput_write_event(this->output_devs_uinput[cfgnum], EV_SYN,
                                            SYN_REPORT, 0);
              }
            }
          }
        }
      }
    }
  } while (rc == LIBEVDEV_READ_STATUS_SYNC ||
           rc == LIBEVDEV_READ_STATUS_SUCCESS || rc == -EAGAIN);
}
