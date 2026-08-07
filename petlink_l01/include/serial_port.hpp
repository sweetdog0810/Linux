#pragma once

#include <atomic>
#include <cstddef>
#include <mutex>
#include <string>

class SerialPort {
public:
    SerialPort() = default;
    ~SerialPort();

    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;

    bool open_port(const std::string& path, int baud_rate, std::string& error);
    void close_port();

    bool is_open() const;
    int fd() const;
    std::string path() const;
    int baud_rate() const;

    bool write_text(const std::string& text, std::string& error);
    ssize_t read_some(char* buffer, std::size_t capacity, std::string& error) const;
    bool wait_for_events(short events, int timeout_ms, short& revents, std::string& error) const;

private:
    std::atomic<int> fd_{-1};
    mutable std::mutex metadata_mutex_;
    std::mutex write_mutex_;
    std::string path_;
    int baud_rate_{0};
};
