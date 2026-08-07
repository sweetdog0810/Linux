#pragma once

#include <string>
#include <vector>

struct SerialDevice {
    std::string path;
    bool readable{false};
    bool writable{false};
};

std::vector<SerialDevice> find_serial_devices();
std::string serial_device_to_string(const SerialDevice& device);
