#include "mapper.hpp"

void evdevMapper::parseParameters(int argc, char *argv[]) {
  if (argc == 3) {
    this->in_device_path = argv[1];
    std::ifstream f(argv[2]);
    json data = json::parse(f);
    if (data.is_array()) {
      for (auto &[confignum, config] : data.items()) {
        Config c;
        if (config["device-name"].is_null() ||
            !config["device-name"].is_string()) {
          c.device_name = "evdev mapper device ";
          c.device_name += confignum;
        } else {
          c.device_name = config["device-name"].get<std::string>();
        }
        if (config["device-bus"].is_null() ||
            !config["device-bus"].is_number_integer()) {
          c.device_bus = 0;
        } else {
          c.device_bus = config["device-bus"].get<int>();
        }
        if (config["device-vendor"].is_null() ||
            !config["device-vendor"].is_number_integer()) {
          c.device_vendor = 0;
        } else {
          c.device_vendor = config["device-vendor"].get<int>();
        }
        if (config["device-product"].is_null() ||
            !config["device-product"].is_number_integer()) {
          c.device_product = 0;
        } else {
          c.device_product = config["device-product"].get<int>();
        }
        if (config["device-version"].is_null() ||
            !config["device-version"].is_number_integer()) {
          c.device_version = 0;
        } else {
          c.device_version = config["device-version"].get<int>();
        }
        for (auto &[abs_code, abs_properties] : config["abs-info"].items()) {
        }
        for (auto &[evtype, evcodes] : config["mappings"].items()) {
          int evt = libevdev_event_type_from_name(evtype.c_str());
          std::vector<Map> *ev_vector;
          switch (evt) {
          case EV_KEY:
            ev_vector = &c.mappings.key_events;
            break;
          case EV_REL:
            ev_vector = &c.mappings.rel_events;
            break;
          case EV_ABS:
            ev_vector = &c.mappings.abs_events;
            break;
          case -1:
            std::cout << "Event type " << evtype << " is not valid event type"
                      << std::endl;
            exit(0);
            break;
          }
          for (auto &[evcode, evprops] : evcodes.items()) {
            Map map;
            int evc = libevdev_event_code_from_name(evt, evcode.c_str());
            if (evc == -1) {
              // error
              std::cout << "Event code " << evcode << " is not valid"
                        << std::endl;
              exit(0);
            }
            map.event_code = evc;
            if (!evprops["action-type"].is_null())
              map.action_type = evprops["action-type"].get<std::string>();
            else
              map.action_type = "single";
            for (auto &[oevnum, oevprops] : evprops["events"].items()) {
              Event oev;
              if (!oevprops["type"].is_null())
                oev.type = libevdev_event_type_from_name(
                    oevprops["type"].get<std::string>().c_str());
              else {
                std::cout << "Event type not specified" << std::endl;
                exit(0);
              }
              if (oev.type == -1) {
                std::cout << "Event type "
                          << oevprops["type"].get<std::string>()
                          << " is not valid event type" << std::endl;
                exit(0);
              }
              if (!oevprops["code"].is_null())
                oev.code = libevdev_event_code_from_name(
                    oev.type, oevprops["code"].get<std::string>().c_str());
              else {
                std::cout << "Event code not specified" << std::endl;
                exit(0);
              }
              if (oev.code == -1) {
                std::cout << "Event code "
                          << oevprops["code"].get<std::string>()
                          << " is not valid" << std::endl;
                exit(0);
              }
              if (!oevprops["value"].is_null())
                oev.value = oevprops["value"].get<int>();
              else
                oev.value = 0;
              map.events.push_back(oev);
            }
            if(map.events.size() != 0)
              ev_vector->push_back(map);
          }
        }
        if(c.mappings.key_events.size() && c.mappings.rel_events.size() && c.mappings.abs_events.size())
          this->configs.push_back(c);
      }
      if (this->configs.size() == 0) {
        std::cout << "Incompatible configuration" << std::endl;
        exit(0);
      }
    } else {
      std::cout << "Incompatible configuration" << std::endl;
      exit(0);
    }
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
  for (auto &config : this->configs) {
    libevdev_free(config.output_dev);
    libevdev_uinput_destroy(config.output_dev_uinput);
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
  for (auto &c : this->configs) {
    for (auto &map : c.mappings.key_events) {
      if (!libevdev_has_event_code(this->input_dev, EV_KEY, map.event_code)) {
        std::cout << "This device doesn't have "
                  << libevdev_event_type_get_name(EV_KEY) << "(" << EV_KEY
                  << ")"
                  << " or "
                  << libevdev_event_code_get_name(EV_KEY, map.event_code) << "("
                  << map.event_code << ")" << std::endl;
        exit(0);
      }
    }
    for (auto &map : c.mappings.rel_events) {
      if (!libevdev_has_event_code(this->input_dev, EV_REL, map.event_code)) {
        std::cout << "This device doesn't have "
                  << libevdev_event_type_get_name(EV_REL) << "(" << EV_REL
                  << ")"
                  << " or "
                  << libevdev_event_code_get_name(EV_REL, map.event_code) << "("
                  << map.event_code << ")" << std::endl;
        exit(0);
      }
    }
    for (auto &map : c.mappings.abs_events) {
      if (!libevdev_has_event_code(this->input_dev, EV_ABS, map.event_code)) {
        std::cout << "This device doesn't have "
                  << libevdev_event_type_get_name(EV_ABS) << "(" << EV_ABS
                  << ")"
                  << " or "
                  << libevdev_event_code_get_name(EV_ABS, map.event_code) << "("
                  << map.event_code << ")" << std::endl;
        exit(0);
      }
    }
  }
}
void evdevMapper::setupOutputDev() {
  for (auto &c : this->configs) {
    c.output_dev = libevdev_new();
    c.output_dev_uinput = nullptr;
    if (!c.output_dev) {
      std::cout << "Failed to allocate memory for device" << std::endl;
      exit(0);
    }
    // Setup device ids
    libevdev_set_name(c.output_dev, c.device_name.c_str());
    libevdev_set_id_bustype(c.output_dev, c.device_bus);
    libevdev_set_id_vendor(c.output_dev, c.device_vendor);
    libevdev_set_id_product(c.output_dev, c.device_product);
    libevdev_set_id_version(c.output_dev, c.device_version);
    for (auto &map : c.mappings.key_events) {
      for (auto event : map.events) {
        libevdev_enable_event_code(c.output_dev, event.type, event.code,
                                   nullptr);
      }
    }
    for (auto &map : c.mappings.rel_events) {
      for (auto event : map.events) {
      }
    }
    for (auto &map : c.mappings.abs_events) {
      for (auto event : map.events) {
      }
    }
    // Create device
    libevdev_uinput_create_from_device(
        c.output_dev, LIBEVDEV_UINPUT_OPEN_MANAGED, &c.output_dev_uinput);
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
      int eventProcessed = 0;
      for (auto &config : this->configs) {
        switch (ev.type) {
        case EV_KEY:
          for (auto &map : config.mappings.key_events) {
            if (map.event_code == ev.code) {
              for (auto event : map.events) {
                switch (event.type) {
                case EV_KEY:
                  libevdev_uinput_write_event(config.output_dev_uinput,
                                              event.type, event.code, ev.value);
                  libevdev_uinput_write_event(config.output_dev_uinput, EV_SYN,
                                              SYN_REPORT, 0);
                  break;
                case EV_REL:
                  if (ev.value > 0) {
                    libevdev_uinput_write_event(config.output_dev_uinput,
                                                event.type, event.code,
                                                event.value);
                    libevdev_uinput_write_event(config.output_dev_uinput,
                                                EV_SYN, SYN_REPORT, 0);
                  }
                  break;
                case EV_ABS:
                  if (ev.value > 0) {
                    libevdev_uinput_write_event(config.output_dev_uinput,
                                                event.type, event.code,
                                                event.value);
                    libevdev_uinput_write_event(config.output_dev_uinput,
                                                EV_SYN, SYN_REPORT, 0);
                  } else {
                    int center = 0;
                    for (auto &absInfo : config.abs_info) {
                      if (absInfo.evcode == event.code) {
                        center = (absInfo.max + absInfo.min) / 2;
                        break;
                      }
                    }
                    libevdev_uinput_write_event(config.output_dev_uinput,
                                                event.type, event.code, center);
                    libevdev_uinput_write_event(config.output_dev_uinput,
                                                EV_SYN, SYN_REPORT, 0);
                  }
                  break;
                }
                if (map.action_type == "single")
                  break;
              }
              eventProcessed = 1;
              break;
            }
          }
          break;
        case EV_REL:
          for (auto &map : config.mappings.rel_events) {
            if (map.event_code == ev.code) {
              for (auto event : map.events) {
                if (map.action_type == "single")
                  break;
              }
              eventProcessed = 1;
              break;
            }
          }
          break;
        case EV_ABS:
          for (auto &map : config.mappings.abs_events) {
            if (map.event_code == ev.code) {
              for (auto event : map.events) {
                int center =
                    (libevdev_get_abs_maximum(this->input_dev, event.code) +
                     libevdev_get_abs_minimum(this->input_dev, event.code)) /
                    2;
                switch (event.type) {
                case EV_KEY:
                  if (ev.value > center + map.deadzone ||
                      ev.value < center - map.deadzone) {
                    libevdev_uinput_write_event(config.output_dev_uinput,
                                                event.type, event.code, 1);
                    libevdev_uinput_write_event(config.output_dev_uinput,
                                                EV_SYN, SYN_REPORT, 0);
                  } else {
                    libevdev_uinput_write_event(config.output_dev_uinput,
                                                event.type, event.code, 0);
                    libevdev_uinput_write_event(config.output_dev_uinput,
                                                EV_SYN, SYN_REPORT, 0);
                  }
                  break;
                case EV_REL:
                  break;
                case EV_ABS:
                  break;
                }
                if (map.action_type == "single")
                  break;
              }
              eventProcessed = 1;
              break;
            }
          }
          break;
        }
        if (eventProcessed)
          break;
      }
    }
  } while (rc == LIBEVDEV_READ_STATUS_SYNC ||
           rc == LIBEVDEV_READ_STATUS_SUCCESS || rc == -EAGAIN);
}
