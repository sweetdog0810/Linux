#pragma once

#include <string>

enum class CommandType {
    Help,
    Status,
    SetFace,
    SetServo,
    SetLight,
    SystemInfo,
    Quit,
    Invalid
};

struct Command {
    CommandType type{CommandType::Invalid};
    std::string text_value{};
    int number_value{0};
    std::string error{};
};

Command parse_command(const std::string& input);
std::string command_type_to_string(CommandType type);
