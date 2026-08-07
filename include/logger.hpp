#pragma once

#include <string>

class Logger {
public:
    explicit Logger(const std::string& path);
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    bool is_open() const;
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);

private:
    int fd_{-1};

    void write_line(const std::string& level, const std::string& message);
    bool write_all(const char* data, std::size_t length);
};
