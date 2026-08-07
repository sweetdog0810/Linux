#include "serial_port.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

namespace {
bool baud_to_speed(int baud_rate, speed_t& speed)
{
    switch (baud_rate) {
    case 1200: speed = B1200; return true;
    case 2400: speed = B2400; return true;
    case 4800: speed = B4800; return true;
    case 9600: speed = B9600; return true;
    case 19200: speed = B19200; return true;
    case 38400: speed = B38400; return true;
    case 57600: speed = B57600; return true;
    case 115200: speed = B115200; return true;
#ifdef B230400
    case 230400: speed = B230400; return true;
#endif
    default: return false;
    }
}

bool configure_8n1_raw(int fd, int baud_rate, std::string& error)
{
    speed_t speed{};
    if (!baud_to_speed(baud_rate, speed)) {
        error = "不支持的波特率：" + std::to_string(baud_rate);
        return false;
    }

    termios options{};
    if (::tcgetattr(fd, &options) != 0) {
        error = std::string("tcgetattr 失败：") + std::strerror(errno);
        return false;
    }

    ::cfmakeraw(&options);

    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
#ifdef CRTSCTS
    options.c_cflag &= ~CRTSCTS;
#endif

    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 0;

    if (::cfsetispeed(&options, speed) != 0 || ::cfsetospeed(&options, speed) != 0) {
        error = std::string("设置波特率失败：") + std::strerror(errno);
        return false;
    }

    if (::tcflush(fd, TCIOFLUSH) != 0) {
        error = std::string("tcflush 失败：") + std::strerror(errno);
        return false;
    }

    if (::tcsetattr(fd, TCSANOW, &options) != 0) {
        error = std::string("tcsetattr 失败：") + std::strerror(errno);
        return false;
    }

    return true;
}
}  // namespace

SerialPort::~SerialPort()
{
    close_port();
}

bool SerialPort::open_port(const std::string& path, int baud_rate, std::string& error)
{
    close_port();

    const int new_fd = ::open(path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (new_fd < 0) {
        error = "open(" + path + ") 失败：" + std::strerror(errno);
        return false;
    }

    if (!configure_8n1_raw(new_fd, baud_rate, error)) {
        ::close(new_fd);
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(metadata_mutex_);
        path_ = path;
        baud_rate_ = baud_rate;
    }
    fd_.store(new_fd);
    return true;
}

void SerialPort::close_port()
{
    std::lock_guard<std::mutex> write_lock(write_mutex_);
    const int old_fd = fd_.exchange(-1);
    if (old_fd >= 0) {
        ::close(old_fd);
    }

    std::lock_guard<std::mutex> meta_lock(metadata_mutex_);
    path_.clear();
    baud_rate_ = 0;
}

bool SerialPort::is_open() const
{
    return fd_.load() >= 0;
}

int SerialPort::fd() const
{
    return fd_.load();
}

std::string SerialPort::path() const
{
    std::lock_guard<std::mutex> lock(metadata_mutex_);
    return path_;
}

int SerialPort::baud_rate() const
{
    std::lock_guard<std::mutex> lock(metadata_mutex_);
    return baud_rate_;
}

bool SerialPort::wait_for_events(short events, int timeout_ms, short& revents, std::string& error) const
{
    const int current_fd = fd_.load();
    if (current_fd < 0) {
        error = "串口未打开";
        return false;
    }

    pollfd descriptor{};
    descriptor.fd = current_fd;
    descriptor.events = events;

    while (true) {
        const int rc = ::poll(&descriptor, 1, timeout_ms);
        if (rc > 0) {
            revents = descriptor.revents;
            return true;
        }
        if (rc == 0) {
            revents = 0;
            return true;
        }
        if (errno == EINTR) {
            continue;
        }
        error = std::string("poll 失败：") + std::strerror(errno);
        return false;
    }
}

bool SerialPort::write_text(const std::string& text, std::string& error)
{
    std::lock_guard<std::mutex> lock(write_mutex_);
    const int current_fd = fd_.load();
    if (current_fd < 0) {
        error = "串口未打开";
        return false;
    }

    std::size_t total = 0;
    while (total < text.size()) {
        const ssize_t count = ::write(current_fd, text.data() + total, text.size() - total);
        if (count > 0) {
            total += static_cast<std::size_t>(count);
            continue;
        }
        if (count < 0 && errno == EINTR) {
            continue;
        }
        if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            pollfd descriptor{};
            descriptor.fd = current_fd;
            descriptor.events = POLLOUT;
            const int rc = ::poll(&descriptor, 1, 500);
            if (rc > 0) {
                continue;
            }
            if (rc == 0) {
                error = "写串口超时";
                return false;
            }
            if (errno == EINTR) {
                continue;
            }
        }

        error = std::string("write 失败：") + std::strerror(errno);
        return false;
    }
    return true;
}

ssize_t SerialPort::read_some(char* buffer, std::size_t capacity, std::string& error) const
{
    const int current_fd = fd_.load();
    if (current_fd < 0) {
        error = "串口未打开";
        return -1;
    }

    while (true) {
        const ssize_t count = ::read(current_fd, buffer, capacity);
        if (count >= 0) {
            return count;
        }
        if (errno == EINTR) {
            continue;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        error = std::string("read 失败：") + std::strerror(errno);
        return -1;
    }
}
