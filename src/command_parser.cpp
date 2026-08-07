#include "command_parser.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace {
std::string to_lower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

bool has_extra_token(std::istringstream& stream)
{
    std::string extra;
    return static_cast<bool>(stream >> extra);
}
}  // namespace

Command parse_command(const std::string& input)
{
    std::istringstream stream(input);
    std::string name;

    if (!(stream >> name)) {
        return {CommandType::Invalid, {}, 0, "输入为空"};
    }

    name = to_lower(name);

    if (name == "help") {
        return has_extra_token(stream)
                   ? Command{CommandType::Invalid, {}, 0, "help 命令不需要参数"}
                   : Command{CommandType::Help};
    }

    if (name == "status") {
        return has_extra_token(stream)
                   ? Command{CommandType::Invalid, {}, 0, "status 命令不需要参数"}
                   : Command{CommandType::Status};
    }

    if (name == "sysinfo") {
        return has_extra_token(stream)
                   ? Command{CommandType::Invalid, {}, 0, "sysinfo 命令不需要参数"}
                   : Command{CommandType::SystemInfo};
    }

    if (name == "quit" || name == "exit") {
        return has_extra_token(stream)
                   ? Command{CommandType::Invalid, {}, 0, "quit 命令不需要参数"}
                   : Command{CommandType::Quit};
    }

    if (name == "face") {
        std::string mode;
        if (!(stream >> mode)) {
            return {CommandType::Invalid, {}, 0, "face 后面需要 normal/happy/angry/sleep/sad"};
        }
        if (has_extra_token(stream)) {
            return {CommandType::Invalid, {}, 0, "face 命令参数过多"};
        }
        mode = to_lower(mode);
        if (mode != "normal" && mode != "happy" && mode != "angry" && mode != "sleep"&& mode != "sad") {
            return {CommandType::Invalid, {}, 0, "不支持的表情：" + mode};
        }
        return {CommandType::SetFace, mode};
    }

    if (name == "servo" || name == "light") {
        int value = 0;
        if (!(stream >> value)) {
            return {CommandType::Invalid, {}, 0, name + " 后面需要整数参数"};
        }
        if (has_extra_token(stream)) {
            return {CommandType::Invalid, {}, 0, name + " 命令参数过多"};
        }
        return {name == "servo" ? CommandType::SetServo : CommandType::SetLight, {}, value};
    }

    return {CommandType::Invalid, {}, 0, "未知命令：" + name};
}

std::string command_type_to_string(CommandType type)
{
    switch (type) {
    case CommandType::Help: return "HELP";
    case CommandType::Status: return "STATUS";
    case CommandType::SetFace: return "SET_FACE";
    case CommandType::SetServo: return "SET_SERVO";
    case CommandType::SetLight: return "SET_LIGHT";
    case CommandType::SystemInfo: return "SYSTEM_INFO";
    case CommandType::Quit: return "QUIT";
    case CommandType::Invalid: return "INVALID";
    }
    return "UNKNOWN";
}
