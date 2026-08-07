#include "logger.hpp"

#include <cerrno>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace {
std::string current_time_text()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm local_time{};
    localtime_r(&now_time, &local_time);

    std::ostringstream output;
    output << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");
    return output.str();
}
}  // namespace

Logger::Logger(const std::string& path)
{
    fd_ = ::open(path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd_ < 0) {
        std::cerr << "无法打开日志文件 " << path << ": " << std::strerror(errno) << '\n';
    }
}

Logger::~Logger()
{
    if (fd_ >= 0) {
        ::close(fd_);
    }
}

bool Logger::is_open() const
{
    return fd_ >= 0;
}

void Logger::info(const std::string& message)
{
    write_line("INFO", message);
}

void Logger::warn(const std::string& message)
{
    write_line("WARN", message);
}

void Logger::error(const std::string& message)
{
    write_line("ERROR", message);
}

void Logger::write_line(const std::string& level, const std::string& message)
{
    if (fd_ < 0) {
        return;
    }

    const std::string line = current_time_text() + " " + level + " " + message + "\n";
    if (!write_all(line.data(), line.size())) {
        std::cerr << "写日志失败：" << std::strerror(errno) << '\n';
    }
}

bool Logger::write_all(const char* data, std::size_t length)
{
    std::size_t written_total = 0;
    while (written_total < length) {
        const ssize_t written = ::write(fd_, data + written_total, length - written_total);
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        written_total += static_cast<std::size_t>(written);
    }
    return true;
}
