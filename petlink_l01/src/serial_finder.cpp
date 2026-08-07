#include "serial_finder.hpp"

#include <algorithm>
#include <glob.h>
#include <sstream>
#include <unistd.h>

namespace {
void append_glob_matches(const char* pattern, std::vector<std::string>& paths)
{
    glob_t result{};
    const int rc = ::glob(pattern, 0, nullptr, &result);
    if (rc == 0) {
        for (std::size_t i = 0; i < result.gl_pathc; ++i) {
            paths.emplace_back(result.gl_pathv[i]);
        }
    }
    ::globfree(&result);
}
}  // namespace

std::vector<SerialDevice> find_serial_devices()
{
    std::vector<std::string> paths;
    append_glob_matches("/dev/ttyUSB*", paths);
    append_glob_matches("/dev/ttyACM*", paths);

    std::sort(paths.begin(), paths.end());
    paths.erase(std::unique(paths.begin(), paths.end()), paths.end());

    std::vector<SerialDevice> devices;
    devices.reserve(paths.size());
    for (const auto& path : paths) {
        devices.push_back({
            path,
            ::access(path.c_str(), R_OK) == 0,
            ::access(path.c_str(), W_OK) == 0,
        });
    }
    return devices;
}

std::string serial_device_to_string(const SerialDevice& device)
{
    std::ostringstream out;
    out << device.path
        << "  read=" << (device.readable ? "yes" : "no")
        << " write=" << (device.writable ? "yes" : "no");
    return out.str();
}
