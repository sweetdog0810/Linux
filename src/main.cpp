#include "command_parser.hpp"
#include "device_state.hpp"
#include "logger.hpp"
#include "system_info.hpp"

#include <filesystem>
#include <iostream>
#include <string>

namespace {
void print_help()
{
    std::cout
        << "可用命令：\n"
        << "  help                 显示帮助\n"
        << "  status               显示当前桌宠状态\n"
        << "  face normal          设置普通表情\n"
        << "  face happy           设置开心表情\n"
        << "  face angry           设置生气表情\n"
        << "  face sleep           设置睡眠表情\n"
        << "  servo <0-180>        设置模拟舵机角度\n"
        << "  light <0-4095>       设置模拟光照值\n"
        << "  sysinfo              读取 Linux 系统信息\n"
        << "  quit                 正常退出程序\n";
}
}  // namespace

int main()
{
    std::error_code directory_error;
    std::filesystem::create_directories("logs", directory_error);
    if (directory_error) {
        std::cerr << "创建 logs 目录失败：" << directory_error.message() << '\n';
        return 1;
    }

    Logger logger("logs/petlink_l00.log");
    if (!logger.is_open()) {
        return 1;
    }

    DeviceState state;
    logger.info("PetLink L00 started");

    std::cout << "PetLink L00 - Linux 命令行桌宠\n";
    std::cout << "输入 help 查看命令。\n";

    std::string input;
    while (true) {
        std::cout << "pet> " << std::flush;
        if (!std::getline(std::cin, input)) {
            std::cout << "\n标准输入结束，程序正常退出。\n";
            logger.info("standard input closed");
            break;
        }

        const Command command = parse_command(input);
        if (command.type == CommandType::Invalid) {
            std::cout << "错误：" << command.error << '\n';
            logger.warn("invalid command input=\"" + input + "\" reason=\"" + command.error + "\"");
            continue;
        }

        logger.info("command=" + command_type_to_string(command.type) + " raw=\"" + input + "\"");

        if (command.type == CommandType::Help) {
            print_help();
            continue;
        }

        if (command.type == CommandType::Status) {
            std::cout << state_to_string(state) << '\n';
            continue;
        }

        if (command.type == CommandType::SystemInfo) {
            std::cout << build_system_report() << '\n';
            continue;
        }

        if (command.type == CommandType::Quit) {
            std::cout << "程序正在正常退出。\n";
            logger.info("PetLink L00 stopped by user");
            break;
        }

        std::string result;
        if (apply_command(state, command, result)) {
            std::cout << "成功：" << result << '\n';
            logger.info(result);
        } else {
            std::cout << "错误：" << result << '\n';
            logger.warn(result);
        }
    }

    return 0;
}
