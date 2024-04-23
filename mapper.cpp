#include "mapper.hpp"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <linux/input-event-codes.h>
#include <ostream>
#include <sstream>

evdevMapper::MAP evdevMapper::parseCFG(char *str) {
  //EV_KEY.KEY_A:EV_KEY.KEY_B
  //EV_KEY.KEY_A:EV_ABS.ABS_X
  MAP m;
  std::stringstream ss(str);
  ss >> m.in_event_type >> m.in_event_code >> m.out_event_type >>
      m.out_event_code;
  return m;
}
void evdevMapper::parseParameters(int argc, char *argv[]) {
  int parserStatus = 0;
  for (int i = 1; i < argc; i++) {
    switch (parserStatus) {
    case PARSE_DEVICE_PATH:
      in_device_path = argv[i];
      parserStatus = 0;
      break;
    case PARSE_DEVICE_NAME:
      out_device_name = argv[i];
      parserStatus = 0;
      break;
    case PARSE_DEVICE_CONFIG:
      config.push_back(parseCFG(argv[i]));
      parserStatus = 0;
      break;
    default:
      if (parserStatus) {
        std::cout << "Error while parsing arguments \n";
        return;
      }
      if (!strcmp(argv[i], "-d")) {
        parserStatus = PARSE_DEVICE_PATH;
      } else if (!strcmp(argv[i], "-n")) {
        parserStatus = PARSE_DEVICE_NAME;
      } else if (!strcmp(argv[i], "-c")) {
        parserStatus = PARSE_DEVICE_CONFIG;
      } else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
        // Help
        return;
      }
      break;
    }
  }
  if (parserStatus) {
    std::cout << "Error while parsing arguments \n";
    return;
  }
  std::cout << "config" << std::endl;
  for (int i = 0; i < this->config.size(); i++) {
    std::cout << this->config[i].in_event_type << " "
              << this->config[i].in_event_code << " "
              << this->config[i].out_event_type << " "
              << this->config[i].out_event_code << std::endl;
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
  libevdev_free(output_dev);
  libevdev_uinput_destroy(output_dev_uinput);
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
  for (int i = 0; i < this->config.size(); i++) {
    if (!libevdev_has_event_type(input_dev, this->config[i].in_event_type)) {
      std::cout << "This device doesn't have "
                << libevdev_event_type_get_name(this->config[i].in_event_type)
                << std::endl;
      exit(0);
    }
    if (!libevdev_has_event_code(input_dev, this->config[i].in_event_type,
                                 this->config[i].in_event_code)) {
      std::cout << "This device doesn't have "
                << libevdev_event_code_get_name(this->config[i].in_event_type,
                                                this->config[i].in_event_code)
                << std::endl;
      exit(0);
    }
  }
}
void evdevMapper::setupOutputDev() {
  this->output_dev = libevdev_new();
  libevdev_set_name(this->output_dev, this->out_device_name.c_str());
  for (int i = 0; i < this->config.size(); i++) {
    libevdev_enable_event_code(this->output_dev, this->config[i].out_event_type,
                               this->config[i].out_event_code, nullptr);
  }
  libevdev_uinput_create_from_device(
      this->output_dev, LIBEVDEV_UINPUT_OPEN_MANAGED, &this->output_dev_uinput);
}
void evdevMapper::eventLoop() {
  int rc;
  do {
    struct input_event ev;
    rc = libevdev_next_event(
        this->input_dev,
        LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_BLOCKING, &ev);
    if (rc == LIBEVDEV_READ_STATUS_SYNC) {
      // printf("::::::::::::::::::::: dropped ::::::::::::::::::::::\n");
      while (rc == LIBEVDEV_READ_STATUS_SYNC) {
        rc = libevdev_next_event(this->input_dev, LIBEVDEV_READ_FLAG_SYNC, &ev);
      }
      // printf("::::::::::::::::::::: re-synced ::::::::::::::::::::::\n");
    } else if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
      // handle events
      std::cout << "Event:" << std::endl
                << "  " << ev.type << " " << ev.code << " " << ev.value
                << std::endl;
      for (int i = 0; i < this->config.size(); i++) {
        if (ev.type != EV_SYN && ev.type == config[i].in_event_type &&
            ev.code == config[i].in_event_code) {
          std::cout << "  Found cfg:" << std::endl
                    << "    " << ev.type << " " << ev.code << " "
                    << config[i].out_event_type << " "
                    << config[i].out_event_code << std::endl;
          libevdev_uinput_write_event(this->output_dev_uinput,
                                      this->config[i].out_event_type,
                                      this->config[i].out_event_code, ev.value);
          libevdev_uinput_write_event(this->output_dev_uinput, EV_SYN,
                                      SYN_REPORT, 0);
        }
      }
    }
  } while (rc == LIBEVDEV_READ_STATUS_SYNC ||
           rc == LIBEVDEV_READ_STATUS_SUCCESS || rc == -EAGAIN);
}
