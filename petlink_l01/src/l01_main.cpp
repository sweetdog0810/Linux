#include "command_parser.hpp"
#include "device_state.hpp"
#include "logger.hpp"
#include "serial_finder.hpp"
#include "serial_port.hpp"
#include "system_info.hpp"

#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <poll.h>
#include <sstream>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {
volatile std::sig_atomic_t g_stop_requested = 0;
std::mutex g_console_mutex;

void signal_handler(int)
{
    g_stop_requested = 1;
}

bool install_signal_handlers()
{
    struct sigaction action{};
    action.sa_handler = signal_handler;
    ::sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    return ::sigaction(SIGINT, &action, nullptr) == 0 &&
           ::sigaction(SIGTERM, &action, nullptr) == 0;
}

std::string trim(const std::string& text)
{
    const std::size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const std::size_t last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

std::string upper(std::string text)
{
    for (char& ch : text) {
        if (ch >= 'a' && ch <= 'z') ch = static_cast<char>(ch - 'a' + 'A');
    }
    return text;
}

void print_help()
{
    std::lock_guard<std::mutex> lock(g_console_mutex);
    std::cout
        << "\nPetLink L01 命令：\n"
        << "  help                         显示帮助\n"
        << "  ports                        重新扫描 /dev/ttyUSB* 与 /dev/ttyACM*\n"
        << "  connect <设备> [波特率]      连接串口，例如 connect /dev/ttyUSB0 115200\n"
        << "  disconnect                   手动断开串口\n"
        << "  serial                       查看当前串口状态\n"
        << "  face normal|happy|angry|sleep  发送 FACE:<MODE>\\n\n"
        << "  servo <0-180>                发送 SERVO:<ANGLE>\\n\n"
        << "  status                       发送 STATUS?\\n\n"
        << "  light <0-4095>               发送 LIGHTSIM:<VALUE>\\n，仅供模拟器学习\n"
        << "  raw <文本>                    原样发送一行文本，程序自动补 \\n\n"
        << "  sysinfo                      显示 Linux 系统信息\n"
        << "  quit                         正常退出\n\n"
        << "说明：L01 使用临时文本协议学习串口系统编程；L02 才改成二进制协议、CRC、ACK。\n\n";
}

void print_ports()
{
    const auto devices = find_serial_devices();
    std::lock_guard<std::mutex> lock(g_console_mutex);
    std::cout << "Available ports:\n";
    if (devices.empty()) {
        std::cout << "  (未发现 /dev/ttyUSB* 或 /dev/ttyACM*)\n";
    } else {
        for (std::size_t i = 0; i < devices.size(); ++i) {
            std::cout << "  [" << i << "] " << serial_device_to_string(devices[i]) << '\n';
        }
    }
}

struct CliOptions {
    std::string port;
    int baud{115200};
    bool list_only{false};
    bool show_help{false};
};

bool parse_int(const std::string& text, int& value)
{
    char* end = nullptr;
    errno = 0;
    const long parsed = std::strtol(text.c_str(), &end, 10);
    if (errno != 0 || end == text.c_str() || *end != '\0') return false;
    if (parsed < 0 || parsed > 4000000) return false;
    value = static_cast<int>(parsed);
    return true;
}

bool parse_cli(int argc, char** argv, CliOptions& options)
{
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            options.port = argv[++i];
        } else if (arg == "--baud" && i + 1 < argc) {
            if (!parse_int(argv[++i], options.baud)) {
                std::cerr << "无效波特率\n";
                return false;
            }
        } else if (arg == "--list") {
            options.list_only = true;
        } else if (arg == "--help" || arg == "-h") {
            options.show_help = true;
        } else {
            std::cerr << "未知启动参数：" << arg << '\n';
            return false;
        }
    }
    return true;
}

class ReceiverWorker {
public:
    ReceiverWorker(SerialPort& serial, Logger& logger)
        : serial_(serial), logger_(logger)
    {
    }

    ~ReceiverWorker()
    {
        stop();
    }

    void start()
    {
        stop();
        stop_requested_.store(false);
        fault_.store(false);
        thread_ = std::thread(&ReceiverWorker::run, this);
    }

    void stop()
    {
        stop_requested_.store(true);
        if (thread_.joinable()) thread_.join();
    }

    bool faulted() const
    {
        return fault_.load();
    }

    std::string fault_message() const
    {
        std::lock_guard<std::mutex> lock(fault_mutex_);
        return fault_message_;
    }

private:
    void set_fault(const std::string& message)
    {
        {
            std::lock_guard<std::mutex> lock(fault_mutex_);
            fault_message_ = message;
        }
        fault_.store(true);
        stop_requested_.store(true);
        logger_.error("serial receive fault: " + message);
    }

    void emit_line(const std::string& line)
    {
        logger_.info("RX " + line);
        std::lock_guard<std::mutex> lock(g_console_mutex);
        std::cout << "\nRX> " << line << "\npet> " << std::flush;
    }

    void run()
    {
        std::string pending;
        char buffer[512]{};

        while (!stop_requested_.load() && !g_stop_requested && serial_.is_open()) {
            short revents = 0;
            std::string error;
            if (!serial_.wait_for_events(POLLIN | POLLERR | POLLHUP | POLLNVAL, 250, revents, error)) {
                if (!stop_requested_.load()) set_fault(error);
                break;
            }

            if (revents == 0) continue;

            if ((revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
                set_fault("poll 检测到设备断开/错误，revents=" + std::to_string(revents));
                break;
            }

            if ((revents & POLLIN) != 0) {
                const ssize_t count = serial_.read_some(buffer, sizeof(buffer), error);
                if (count < 0) {
                    set_fault(error);
                    break;
                }
                if (count == 0) {
                    continue;
                }

                pending.append(buffer, static_cast<std::size_t>(count));
                while (true) {
                    const std::size_t newline = pending.find('\n');
                    if (newline == std::string::npos) break;
                    std::string line = pending.substr(0, newline);
                    pending.erase(0, newline + 1);
                    if (!line.empty() && line.back() == '\r') line.pop_back();
                    emit_line(line);
                }

                if (pending.size() > 4096) {
                    emit_line("[partial] " + pending.substr(0, 4096));
                    pending.clear();
                }
            }
        }
    }

    SerialPort& serial_;
    Logger& logger_;
    std::atomic<bool> stop_requested_{false};
    std::atomic<bool> fault_{false};
    mutable std::mutex fault_mutex_;
    std::string fault_message_;
    std::thread thread_;
};

bool send_wire(SerialPort& serial, Logger& logger, const std::string& wire)
{
    if (!serial.is_open()) {
        std::lock_guard<std::mutex> lock(g_console_mutex);
        std::cout << "错误：串口未连接。先执行 ports，再 connect <设备> [波特率]。\n";
        return false;
    }

    std::string error;
    if (!serial.write_text(wire, error)) {
        logger.error("TX failed: " + error);
        std::lock_guard<std::mutex> lock(g_console_mutex);
        std::cout << "错误：发送失败：" << error << '\n';
        return false;
    }

    std::string display = wire;
    while (!display.empty() && (display.back() == '\n' || display.back() == '\r')) display.pop_back();
    logger.info("TX " + display);
    std::lock_guard<std::mutex> lock(g_console_mutex);
    std::cout << "TX> " << display << '\n';
    return true;
}

bool connect_serial(SerialPort& serial,
                    ReceiverWorker& receiver,
                    Logger& logger,
                    const std::string& path,
                    int baud)
{
    receiver.stop();
    serial.close_port();

    std::string error;
    if (!serial.open_port(path, baud, error)) {
        logger.error("connect failed path=" + path + " reason=" + error);
        std::lock_guard<std::mutex> lock(g_console_mutex);
        std::cout << "连接失败：" << error << '\n';
        return false;
    }

    logger.info("serial connected path=" + path + " baud=" + std::to_string(baud));
    {
        std::lock_guard<std::mutex> lock(g_console_mutex);
        std::cout << "Connected to " << path << " @ " << baud << " 8N1 raw\n";
    }
    receiver.start();
    return true;
}

void disconnect_serial(SerialPort& serial, ReceiverWorker& receiver, Logger& logger)
{
    const std::string old_path = serial.path();
    receiver.stop();
    serial.close_port();
    if (!old_path.empty()) logger.info("serial disconnected path=" + old_path);
}

bool handle_runtime_command(const std::string& input,
                            SerialPort& serial,
                            ReceiverWorker& receiver,
                            Logger& logger)
{
    const std::string line = trim(input);

    if (line == "ports") {
        print_ports();
        return true;
    }

    if (line == "serial") {
        std::lock_guard<std::mutex> lock(g_console_mutex);
        if (serial.is_open()) {
            std::cout << "Serial: ONLINE path=" << serial.path()
                      << " baud=" << serial.baud_rate() << " 8N1 raw\n";
        } else {
            std::cout << "Serial: OFFLINE\n";
        }
        return true;
    }

    if (line == "disconnect") {
        disconnect_serial(serial, receiver, logger);
        std::lock_guard<std::mutex> lock(g_console_mutex);
        std::cout << "串口已手动断开。\n";
        return true;
    }

    if (line.rfind("connect ", 0) == 0) {
        std::istringstream stream(line);
        std::string command;
        std::string path;
        std::string baud_text;
        std::string extra;
        stream >> command >> path;
        int baud = 115200;
        if (stream >> baud_text) {
            if (!parse_int(baud_text, baud)) {
                std::lock_guard<std::mutex> lock(g_console_mutex);
                std::cout << "错误：波特率必须是整数。\n";
                return true;
            }
        }
        if (stream >> extra) {
            std::lock_guard<std::mutex> lock(g_console_mutex);
            std::cout << "错误：connect 参数过多。\n";
            return true;
        }
        connect_serial(serial, receiver, logger, path, baud);
        return true;
    }

    if (line.rfind("raw ", 0) == 0) {
        const std::string payload = line.substr(4);
        send_wire(serial, logger, payload + "\n");
        return true;
    }

    return false;
}

void maybe_initial_select(SerialPort& serial, ReceiverWorker& receiver, Logger& logger)
{
    const auto devices = find_serial_devices();
    {
        std::lock_guard<std::mutex> lock(g_console_mutex);
        std::cout << "Available ports:\n";
        if (devices.empty()) {
            std::cout << "  (未发现 /dev/ttyUSB* 或 /dev/ttyACM*)\n"
                      << "进入未连接模式。以后可执行 connect /dev/ttyUSB0 115200。\n";
            return;
        }
        for (std::size_t i = 0; i < devices.size(); ++i) {
            std::cout << "  [" << i << "] " << serial_device_to_string(devices[i]) << '\n';
        }
        std::cout << "Select port index [Enter=offline]: " << std::flush;
    }

    std::string selection;
    if (!std::getline(std::cin, selection)) return;
    selection = trim(selection);
    if (selection.empty()) return;

    int index = -1;
    if (!parse_int(selection, index) || index < 0 || static_cast<std::size_t>(index) >= devices.size()) {
        std::lock_guard<std::mutex> lock(g_console_mutex);
        std::cout << "编号无效，进入未连接模式。\n";
        return;
    }

    int baud = 115200;
    {
        std::lock_guard<std::mutex> lock(g_console_mutex);
        std::cout << "Baud rate [115200]: " << std::flush;
    }
    std::string baud_text;
    if (std::getline(std::cin, baud_text)) {
        baud_text = trim(baud_text);
        if (!baud_text.empty() && !parse_int(baud_text, baud)) {
            std::lock_guard<std::mutex> lock(g_console_mutex);
            std::cout << "波特率无效，使用默认 115200。\n";
            baud = 115200;
        }
    }

    connect_serial(serial, receiver, logger, devices[static_cast<std::size_t>(index)].path, baud);
}
}  // namespace

int main(int argc, char** argv)
{
    CliOptions options;
    if (!parse_cli(argc, argv, options)) {
        return argc > 1 ? 1 : 0;
    }

    if (options.show_help) {
        std::cout << "Usage: petlink_l01 [--port /dev/ttyUSB0] [--baud 115200] [--list]\n";
        return 0;
    }

    if (options.list_only) {
        print_ports();
        return 0;
    }

    if (!install_signal_handlers()) {
        std::cerr << "安装 SIGINT/SIGTERM 处理器失败：" << std::strerror(errno) << '\n';
        return 1;
    }

    std::error_code directory_error;
    std::filesystem::create_directories("logs", directory_error);
    if (directory_error) {
        std::cerr << "创建 logs 目录失败：" << directory_error.message() << '\n';
        return 1;
    }

    Logger logger("logs/petlink_l01.log");
    if (!logger.is_open()) return 1;
    logger.info("PetLink L01 started");

    SerialPort serial;
    ReceiverWorker receiver(serial, logger);
    DeviceState desired_state;

    {
        std::lock_guard<std::mutex> lock(g_console_mutex);
        std::cout << "PetLink L01 - Linux Serial Console\n"
                  << "目标：/dev + open/read/write + termios + poll + thread + mutex + signal\n";
    }

    if (!options.port.empty()) {
        connect_serial(serial, receiver, logger, options.port, options.baud);
    } else if (::isatty(STDIN_FILENO)) {
        maybe_initial_select(serial, receiver, logger);
    } else {
        print_ports();
    }

    print_help();

    bool running = true;
    bool prompt_visible = false;
    while (running && !g_stop_requested) {
        if (receiver.faulted()) {
            const std::string fault = receiver.fault_message();
            disconnect_serial(serial, receiver, logger);
            {
                std::lock_guard<std::mutex> lock(g_console_mutex);
                std::cout << "\n串口已断开：" << fault << '\n'
                          << "L01 不自动重连；请重新插入设备后执行 ports / connect。\n";
            }
            prompt_visible = false;
        }

        if (!prompt_visible) {
            std::lock_guard<std::mutex> lock(g_console_mutex);
            std::cout << "pet> " << std::flush;
            prompt_visible = true;
        }

        pollfd stdin_poll{};
        stdin_poll.fd = STDIN_FILENO;
        stdin_poll.events = POLLIN | POLLHUP | POLLERR;
        const int poll_result = ::poll(&stdin_poll, 1, 250);
        if (poll_result < 0) {
            if (errno == EINTR) continue;
            logger.error(std::string("stdin poll failed: ") + std::strerror(errno));
            break;
        }
        if (poll_result == 0) continue;
        if ((stdin_poll.revents & (POLLHUP | POLLERR | POLLNVAL)) != 0 &&
            (stdin_poll.revents & POLLIN) == 0) {
            logger.info("standard input closed");
            break;
        }
        if ((stdin_poll.revents & POLLIN) == 0) continue;

        std::string input;
        if (!std::getline(std::cin, input)) {
            logger.info("standard input closed");
            break;
        }
        prompt_visible = false;

        const std::string clean = trim(input);
        if (clean.empty()) continue;

        if (handle_runtime_command(clean, serial, receiver, logger)) continue;

        const Command command = parse_command(clean);
        if (command.type == CommandType::Invalid) {
            std::lock_guard<std::mutex> lock(g_console_mutex);
            std::cout << "错误：" << command.error << '\n';
            logger.warn("invalid command raw=\"" + clean + "\" reason=\"" + command.error + "\"");
            continue;
        }

        if (command.type == CommandType::Help) {
            print_help();
            continue;
        }
        if (command.type == CommandType::SystemInfo) {
            std::lock_guard<std::mutex> lock(g_console_mutex);
            std::cout << build_system_report() << '\n';
            continue;
        }
        if (command.type == CommandType::Quit) {
            running = false;
            continue;
        }

        if (command.type == CommandType::Status) {
            send_wire(serial, logger, "STATUS?\n");
            continue;
        }

        if (command.type == CommandType::SetFace ||
            command.type == CommandType::SetServo ||
            command.type == CommandType::SetLight) {
            if (!serial.is_open()) {
                std::lock_guard<std::mutex> lock(g_console_mutex);
                std::cout << "错误：串口未连接，命令没有发送。\n";
                continue;
            }

            std::string result;
            if (!apply_command(desired_state, command, result)) {
                std::lock_guard<std::mutex> lock(g_console_mutex);
                std::cout << "错误：" << result << '\n';
                logger.warn(result);
                continue;
            }

            if (command.type == CommandType::SetFace) {
                send_wire(serial, logger, "FACE:" + upper(command.text_value) + "\n");
            } else if (command.type == CommandType::SetServo) {
                send_wire(serial, logger, "SERVO:" + std::to_string(command.number_value) + "\n");
            } else {
                send_wire(serial, logger, "LIGHTSIM:" + std::to_string(command.number_value) + "\n");
            }
            continue;
        }
    }

    if (g_stop_requested) {
        std::lock_guard<std::mutex> lock(g_console_mutex);
        std::cout << "\n收到 SIGINT/SIGTERM，开始正常清理资源...\n";
        logger.info("stop requested by signal");
    }

    disconnect_serial(serial, receiver, logger);
    logger.info("PetLink L01 stopped normally");
    {
        std::lock_guard<std::mutex> lock(g_console_mutex);
        std::cout << "PetLink L01 已正常退出。\n";
    }
    return 0;
}
