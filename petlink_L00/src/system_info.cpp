#include "system_info.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sstream>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>

namespace {
std::string read_text_file(const char* path, std::size_t max_bytes)
{
    const int fd = ::open(path, O_RDONLY);
    if (fd < 0) {
        return std::string("读取失败：") + std::strerror(errno);
    }

    std::string content;
    content.resize(max_bytes);
    const ssize_t count = ::read(fd, content.data(), content.size());
    const int saved_errno = errno;
    ::close(fd);

    if (count < 0) {
        return std::string("读取失败：") + std::strerror(saved_errno);
    }

    content.resize(static_cast<std::size_t>(count));
    return content;
}

std::string first_matching_line(const std::string& text, const std::string& prefix)
{
    std::istringstream input(text);
    std::string line;
    while (std::getline(input, line)) {
        if (line.rfind(prefix, 0) == 0) {
            return line;
        }
    }
    return "未找到 " + prefix;
}
}  // namespace

std::string build_system_report()
{
    char cwd[4096]{};
    const char* cwd_result = ::getcwd(cwd, sizeof(cwd));

    utsname system_name{};
    const bool uname_ok = (::uname(&system_name) == 0);

    const std::string os_release = read_text_file("/etc/os-release", 4096);
    const std::string meminfo = read_text_file("/proc/meminfo", 4096);

    std::ostringstream report;
    report << "--- Linux System Report ---\n"
           << "PID: " << ::getpid() << '\n'
           << "UID: " << ::getuid() << '\n'
           << "GID: " << ::getgid() << '\n'
           << "CWD: " << (cwd_result != nullptr ? cwd : "读取失败") << '\n';

    if (uname_ok) {
        report << "Kernel: " << system_name.sysname << ' ' << system_name.release << '\n'
               << "Machine: " << system_name.machine << '\n';
    } else {
        report << "Kernel: 读取失败\n";
    }

    report << first_matching_line(os_release, "PRETTY_NAME=") << '\n'
           << first_matching_line(meminfo, "MemTotal:");
    return report.str();
}
