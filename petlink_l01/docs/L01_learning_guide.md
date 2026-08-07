# PetLink L01 完整学习指南：Linux 串口系统编程版

> 目标：在 L00 基础上，不依赖 ROS2，完成一个真正的 Linux 用户态串口控制台。
> L01 先使用简单文本协议，把 Linux 串口、线程、poll、信号、权限、日志学扎实；L02 再进入二进制协议、CRC、ACK、重发和自动重连。

---

## 0. L01 完成后你必须能回答的问题

1. 为什么 USB 转 TTL 在 Linux 中表现为 `/dev/ttyUSB0` 或 `/dev/ttyACM0`？
2. `open()` 返回的 fd 到底是什么？
3. 为什么串口可以用 `read/write`，普通文件也可以用 `read/write`？
4. `O_NOCTTY` 与 `O_NONBLOCK` 的作用是什么？
5. 115200、8N1 分别代表什么？
6. `tcgetattr -> 修改 termios -> tcsetattr` 为什么是一个完整配置流程？
7. `cfmakeraw()` 为什么适合二进制/串口原始数据？
8. `poll()` 与直接死循环 `read()` 有什么区别？
9. 为什么主线程和接收线程可以同时运行？
10. 为什么多个线程共享日志、串口发送、控制台输出时需要互斥？
11. Ctrl+C 为什么会产生 SIGINT？
12. 为什么信号处理函数里不能随意使用复杂 C++ 操作？
13. 串口突然拔掉时程序为什么不能崩溃？
14. L01 为什么故意不做自动重连？
15. 为什么 L01 用文本协议，而 L02 再做二进制协议？

---

# 第一阶段：保护 L00，建立 L01 起点

## 1. 回到你的当前工程

```bash
cd ~/Linux/petlink_l00
pwd
```

确认输出是你的 L00 项目目录。

## 2. 先确认 L00 没坏

```bash
make test
```

必须出现：

```text
L00 automatic self-check: PASS
```

如果 L00 都没有通过，不要先进入 L01。

## 3. 保存 L00 Git 版本

```bash
git status
git log --oneline --max-count=5
git tag
```

如果还没有 L00 提交：

```bash
git add .
git commit -m "L00: complete Linux command-line pet"
```

如果还没有 L00 标签：

```bash
git tag L00
```

以后可以比较：

```bash
git diff L00..L01 --stat
```

---

# 第二阶段：先学会在 Ubuntu 中找到真正的串口

## 4. VMware 必须把 USB 设备交给 Ubuntu

你在 VMware 中学习。USB 转 TTL 插入 Windows 主机后，设备可能先被 Windows 占用。

在 VMware 中找到该 USB 设备并选择“Connect / 连接到虚拟机（从主机断开）”。也可以观察 VMware 窗口底部的 USB 图标。

如果 USB 根本没交给 Ubuntu，那么 Linux 程序写得完全正确也找不到 `/dev/ttyUSB0`。

## 5. 插入前记录一次

```bash
lsusb
ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
```

## 6. 把 USB 转 TTL 连接给虚拟机后再次执行

```bash
lsusb
ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
```

常见结果：

```text
/dev/ttyUSB0
```

或者：

```text
/dev/ttyACM0
```

## 7. 查看内核日志

```bash
sudo dmesg | tail -n 30
```

也可以先运行：

```bash
sudo dmesg -w
```

然后拔插 USB，实时观察内核创建/移除 tty 设备。

按 Ctrl+C 退出 `dmesg -w`。

## 8. 查看设备权限

假设设备是 `/dev/ttyUSB0`：

```bash
ls -l /dev/ttyUSB0
```

典型：

```text
crw-rw---- 1 root dialout ... /dev/ttyUSB0
```

第一个字符 `c` 代表字符设备。

查看自己的组：

```bash
groups
id
```

如果没有 `dialout`：

```bash
sudo usermod -aG dialout $USER
```

然后注销 Ubuntu 用户再登录，或重启虚拟机。再次：

```bash
groups
```

应出现 `dialout`。

不要把 `sudo ./petlink_l01` 当成长期解决方式，也不要习惯性 `chmod 777 /dev/ttyUSB0`。

## 9. 使用 stty 查看串口参数

```bash
stty -F /dev/ttyUSB0 -a
```

若是 ttyACM：

```bash
stty -F /dev/ttyACM0 -a
```

重点观察：

- speed
- cs8
- parenb / -parenb
- cstopb / -cstopb

本项目最终由 `termios` 在程序内部完成配置，不要求依赖 stty 设置。

---

# 第三阶段：理解 L01 增加了哪些代码

L01 在 L00 基础上增加：

```text
include/
├── serial_finder.hpp       新增
└── serial_port.hpp         新增

src/
├── serial_finder.cpp       新增
├── serial_port.cpp         新增
└── l01_main.cpp            新增

tools/
└── stm32_text_simulator.py 新增

scripts/
├── l01_static_check.sh
├── l01_integration_check.py
└── l01_disconnect_check.py
```

同时修改：

```text
logger.hpp / logger.cpp     增加线程安全互斥
CMakeLists.txt              同时构建 L00 与 L01
Makefile                    增加 L01 构建/模拟/测试命令
.vscode/tasks.json          增加 L01 按钮任务
.vscode/launch.json         增加 L01 GDB 配置
```

保留 `petlink_l00` 可执行文件，是为了做回归测试；新的主程序叫 `petlink_l01`。

---

# 第四阶段：学习 serial_finder——Linux 如何“找到串口”

## 10. 打开文件

```bash
code include/serial_finder.hpp src/serial_finder.cpp
```

### 你要看懂的数据结构

```cpp
struct SerialDevice {
    std::string path;
    bool readable;
    bool writable;
};
```

每个候选设备记录：

- 路径
- 当前用户是否可读
- 当前用户是否可写

### 扫描的模式

```text
/dev/ttyUSB*
/dev/ttyACM*
```

程序使用 `glob()` 做通配符扫描，再使用：

```cpp
access(path.c_str(), R_OK)
access(path.c_str(), W_OK)
```

做权限提示。

注意：`access()` 只是提前提示，真正能不能打开仍以 `open()` 的返回值为准。

## 11. 编译并列出串口

```bash
make build
make list
```

等价于：

```bash
./build/petlink_l01 --list
```

真实设备存在时可能显示：

```text
Available ports:
  [0] /dev/ttyUSB0 read=yes write=yes
```

没有设备时：

```text
Available ports:
  (未发现 /dev/ttyUSB* 或 /dev/ttyACM*)
```

这不是程序崩溃，而是合法的“无设备状态”。

---

# 第五阶段：学习 SerialPort——L01 最核心模块

## 12. 打开 SerialPort

```bash
code include/serial_port.hpp src/serial_port.cpp
```

先找到：

```cpp
bool SerialPort::open_port(...)
```

### 12.1 open()

核心：

```cpp
open(path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK)
```

含义：

- `O_RDWR`：同时读写串口
- `O_NOCTTY`：不要让这个串口成为当前进程的控制终端
- `O_NONBLOCK`：采用非阻塞方式，配合 `poll()` 等待就绪

成功：返回 fd >= 0。
失败：返回 -1，并设置 `errno`。

### 12.2 文件描述符

假设：

```text
0 -> stdin
1 -> stdout
2 -> stderr
3 -> 日志文件
4 -> /dev/ttyUSB0
```

fd 不是“串口数据”，而是当前进程访问内核对象的一个整数句柄。

## 13. termios 配置流程

在 `configure_8n1_raw()` 中按顺序看：

```text
tcgetattr(fd, &options)
        ↓
cfmakeraw(&options)
        ↓
设置 CLOCAL / CREAD / CS8
清除 PARENB / CSTOPB / CRTSCTS
        ↓
cfsetispeed / cfsetospeed
        ↓
tcflush
        ↓
tcsetattr(fd, TCSANOW, &options)
```

### 13.1 115200 8N1

```text
115200 = 波特率
8      = 8 个数据位
N      = No parity，无校验
1      = 1 个停止位
```

### 13.2 raw mode

终端设备默认可能会做：

- 行编辑
- 回显
- 特殊字符处理
- 换行转换

串口通信通常希望收到什么字节就看到什么字节，所以使用：

```cpp
cfmakeraw(&options);
```

### 13.3 波特率不是直接填整数

`termios` 使用：

```cpp
B9600
B115200
```

所以项目中 `baud_to_speed()` 把整数 115200 转成 `B115200`。

### 13.4 tcsetattr 才真正生效

`cfsetispeed/cfsetospeed` 只修改内存里的 termios 结构体；最后必须：

```cpp
tcsetattr(fd, TCSANOW, &options)
```

把配置交给内核。

---

# 第六阶段：read/write 与 poll

## 14. 发送数据

找到：

```cpp
SerialPort::write_text()
```

不要认为一次：

```cpp
write(fd, data, length)
```

必然写完 `length` 字节。

项目用循环累计：

```text
已写 total
    ↓
继续写剩余部分
    ↓
直到全部写完
```

若返回 `EAGAIN/EWOULDBLOCK`，先用：

```cpp
poll(... POLLOUT ...)
```

等待串口变为可写。

## 15. 接收数据

找到：

```cpp
SerialPort::read_some()
```

可能结果：

```text
>0   实际读取字节数
0    当前没有可读数据
-1   真正错误
```

## 16. 为什么不用 while(1) 疯狂 read

错误思路：

```cpp
while (true) {
    read(fd, ...);
}
```

如果非阻塞，可能不停返回 EAGAIN，CPU 空转。
如果阻塞，线程可能永远卡在 read，不能及时检查退出。

项目使用：

```cpp
poll(fd, ..., 250ms)
```

只有就绪再 `read()`。

L01 需要认识的事件：

```text
POLLIN   可读
POLLOUT  可写
POLLERR  错误
POLLHUP  挂断/设备离开
POLLNVAL fd 无效
```

---

# 第七阶段：线程和互斥锁

## 17. 为什么需要接收线程

L01 设计：

```text
主线程
├── poll(stdin)
├── 接收用户命令
└── 向串口 write

接收线程
├── poll(serial fd)
├── read 串口
└── 打印 RX> ...
```

这样即使 STM32 随时主动上报状态，Linux 也可以及时接收，而主线程仍可等待用户操作。

## 18. C++ std::thread 与书中 pthread

教材使用 POSIX：

```text
pthread_create
pthread_join
pthread_exit
```

本项目是 C++17，所以使用：

```cpp
std::thread
thread.join()
```

底层仍依赖 Linux 线程能力。L01 重点先掌握：

- 一个进程内部有多个执行流
- 线程共享进程的 fd、内存等资源
- 退出前要正确 join

## 19. 为什么要互斥锁

L01 有多个执行流会碰到：

- 日志文件
- 控制台 `std::cout`
- 串口写入/关闭

如果不保护，可能交叉输出或发生竞争。

项目中使用：

```cpp
std::mutex
std::lock_guard<std::mutex>
```

它对应教材 POSIX mutex 的核心思想：同一时刻只允许一个线程进入临界区。

特别观察：

```text
Logger::mutex_
SerialPort::write_mutex_
g_console_mutex
```

---

# 第八阶段：信号与 Ctrl+C 正常退出

## 20. SIGINT

在终端按：

```text
Ctrl+C
```

通常会给前台程序发送：

```text
SIGINT
```

项目用：

```cpp
sigaction(SIGINT, ...)
sigaction(SIGTERM, ...)
```

捕获它。

## 21. 信号处理函数为什么只设置一个标志

项目处理器：

```cpp
g_stop_requested = 1;
```

不会在信号处理函数里：

- 复杂输出
- 获取 mutex
- join 线程
- 操作 STL 容器

原因是信号处理发生在异步时刻，只有少量函数是 async-signal-safe。

真正的清理放在正常主流程：

```text
检测 stop flag
    ↓
receiver.stop()
    ↓
join 接收线程
    ↓
close 串口 fd
    ↓
写最终日志
    ↓
main return
```

---

# 第九阶段：先不用 STM32，用 PTY 模拟器把 Linux 学透

这是 L01 很重要的一步。

## 22. 终端 A：启动模拟 STM32

```bash
cd ~/Linux/petlink_l00
make sim
```

会打印类似：

```text
Pseudo serial device: /dev/pts/5
```

不要关闭这个终端。

`/dev/pts/5` 是 Linux 伪终端，它可以像 tty 一样被 `open/read/write/termios` 操作，因此非常适合自动测试串口程序。

## 23. 终端 B：连接模拟串口

把编号替换成终端 A 实际输出：

```bash
./build/petlink_l01 --port /dev/pts/5 --baud 115200
```

应该看到：

```text
Connected to /dev/pts/5 @ 115200 8N1 raw
```

## 24. 测试 face

输入：

```text
face happy
```

Linux：

```text
TX> FACE:HAPPY
```

模拟 STM32 回复：

```text
RX> STM32: OK FACE=HAPPY
```

## 25. 测试 servo

```text
servo 120
```

预期：

```text
TX> SERVO:120
RX> STM32: OK SERVO=120
```

测试错误：

```text
servo 181
```

必须被 Linux 在发送前拒绝。

## 26. 测试状态查询

```text
status
```

预期：

```text
TX> STATUS?
RX> STATE mode=happy light=1823 servo=120
```

## 27. raw 原始发送

```text
raw PING
```

预期：

```text
TX> PING
RX> PONG
```

## 28. 查看串口状态

```text
serial
```

预期：

```text
Serial: ONLINE path=/dev/pts/5 baud=115200 8N1 raw
```

## 29. 正常退出

```text
quit
```

应看到：

```text
PetLink L01 已正常退出。
```

---

# 第十阶段：测试串口断开，程序必须活着

## 30. 再次启动模拟器与 L01

先正常连接。

然后直接在模拟器终端 A 按：

```text
Ctrl+C
```

这相当于串口另一端突然消失。

L01 应提示串口断开，程序本身不能崩溃。

然后输入：

```text
serial
```

应为：

```text
Serial: OFFLINE
```

再执行：

```text
quit
```

仍然应正常退出。

重要：

> L01 只负责检测断线并活下来；自动扫描和重连属于 L02。

---

# 第十一阶段：使用真实 USB 转 TTL + STM32

## 31. 硬件连接基本原则

- USB-TTL TX -> MCU RX
- USB-TTL RX -> MCU TX
- GND -> GND，共地
- 确认 TTL 电平适合 MCU（常见 STM32 使用 3.3V 逻辑）
- 如果开发板已有独立供电，不要在不确定情况下同时接多路电源

## 32. 确认 VMware 识别

```bash
lsusb
make list
ls -l /dev/ttyUSB0
```

## 33. 运行

```bash
./build/petlink_l01 --port /dev/ttyUSB0 --baud 115200
```

或者无参数运行，让程序显示可用端口并选择：

```bash
./build/petlink_l01
```

## 34. 重要：你的 STM32 端必须理解 L01 临时文本协议

Linux 发送：

```text
FACE:HAPPY\n
SERVO:90\n
STATUS?\n
```

如果你当前 STM32 固件没有这些命令，那么 Linux TX 正常，但不会得到预期 RX，这不代表 Linux 串口代码错误。

因此学习顺序建议：

1. 先用 PTY 模拟器确认 Linux L01 完全通过；
2. 再给 STM32 临时增加简单的文本响应；
3. 最后进入 L02 时把临时文本协议换成正式二进制协议。

---

# 第十二阶段：常用 Linux 串口排错命令

## 35. 找设备

```bash
lsusb
ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
make list
```

## 36. 看权限

```bash
ls -l /dev/ttyUSB0
groups
id
```

## 37. 看参数

```bash
stty -F /dev/ttyUSB0 -a
```

## 38. 看内核插拔日志

```bash
sudo dmesg -w
```

## 39. 查看哪个进程占着串口

```bash
lsof /dev/ttyUSB0
```

若没有 lsof：

```bash
sudo apt install -y lsof
```

也可以：

```bash
fuser /dev/ttyUSB0
```

## 40. 检查进程和线程

先找到 PID：

```bash
pgrep -n petlink_l01
```

假设 PID 是 1234：

```bash
ps -L -p 1234 -o pid,tid,stat,comm
```

观察线程：

```bash
top -H -p 1234
```

退出 top：按 `q`。

---

# 第十三阶段：使用 strace 看见内核调用

## 41. 无硬件可用 PTY 测试

先启动模拟器，然后：

```bash
strace -f \
  -e trace=openat,read,write,poll,close,ioctl \
  ./build/petlink_l01 --port /dev/pts/5 --baud 115200
```

你会看到：

```text
openat(... /dev/pts/5 ...)
ioctl(...)
poll(...)
read(...)
write(...)
close(...)
```

重点：

`tcgetattr/tcsetattr` 在 libc 下面通常通过 `ioctl` 与 tty 驱动交互，所以 strace 中可能看到的是 ioctl。

保存：

```bash
strace -f -o l01_strace.log \
  -e trace=openat,read,write,poll,close,ioctl \
  ./build/petlink_l01 --port /dev/pts/5 --baud 115200
```

查看：

```bash
less l01_strace.log
```

---

# 第十四阶段：GDB 调试 L01

## 42. 启动模拟器

```bash
make sim
```

记住 PTY 路径，例如 `/dev/pts/5`。

## 43. 打开 GDB

```bash
gdb ./build/petlink_l01
```

## 44. 在串口打开处断点

```gdb
break SerialPort::open_port
```

运行：

```gdb
run --port /dev/pts/5 --baud 115200
```

查看参数：

```gdb
print path
print baud_rate
```

继续：

```gdb
continue
```

## 45. 在发送处断点

```gdb
break SerialPort::write_text
continue
```

输入：

```text
face happy
```

程序停下后：

```gdb
print text
backtrace
```

## 46. 看线程

```gdb
info threads
```

切线程：

```gdb
thread 2
bt
```

## 47. 退出

```gdb
quit
```

---

# 第十五阶段：日志

运行 L01 后：

```bash
cat logs/petlink_l01.log
```

实时看：

```bash
tail -f logs/petlink_l01.log
```

应记录：

```text
启动
连接
TX
RX
连接失败
read/write/poll 错误
断开
正常退出
```

由于主线程和接收线程都会写日志，所以 Logger 在 L01 被改造成带 mutex 的线程安全版本。

---

# 第十六阶段：必须做的故障实验

## 48. 不存在的串口

```bash
./build/petlink_l01 --port /dev/not_exist --baud 115200
```

要求：

- 显示 open 失败原因
- 程序仍可进入控制台
- 可以输入 `quit` 正常退出

## 49. 不支持波特率

```bash
./build/petlink_l01 --port /dev/ttyUSB0 --baud 12345
```

应显示“不支持的波特率”。

## 50. 权限不足

若真实设备 `ls -l` 显示属于 dialout，而当前用户不在 dialout，程序应报告 Permission denied。

解决流程：

```bash
groups
sudo usermod -aG dialout $USER
# 注销重新登录
```

不要把程序改成必须 sudo 运行。

## 51. 设备被占用

先开 minicom 或另一个程序占据串口，再测试 PetLink。

用：

```bash
lsof /dev/ttyUSB0
```

定位占用者。

注意：某些 tty 驱动允许多个 open，所以“被占用”并不总是必然 open 失败；但多个程序同时读写会造成数据混乱，因此实际项目应避免这种情况。

## 52. 运行中拔掉 USB

要求：

- 不崩溃
- 检测到错误/挂断
- `serial` 变 OFFLINE
- 可以 `quit`
- L01 不自动重连

---

# 第十七阶段：自动自检

## 53. 一条命令

```bash
make test
```

它会执行三层检查：

```text
L00 回归测试
    ↓
L01 源码静态检查
    ↓
PTY 模拟 STM32 收发集成测试
    ↓
PTY 突然断线测试
```

必须全部 PASS。

## 54. 单独测试

```bash
make test-l00
make test-l01
```

`test-l01` 检查：

- tcgetattr/tcsetattr
- cfsetispeed/cfsetospeed
- poll
- receiver thread
- mutex
- sigaction
- ttyUSB/ttyACM 扫描
- PTY 真实 open/termios/write/read
- 断线后继续存活

---

# 第十八阶段：Git 完成 L01

## 55. 查看修改

```bash
git status
git diff --stat
```

## 56. 自检必须通过

```bash
make test
```

## 57. 提交

```bash
git add .
git commit -m "L01: complete Linux serial console"
```

## 58. 打标签

```bash
git tag L01
```

查看：

```bash
git log --oneline --decorate --graph --all
git tag
```

比较 L00 到 L01：

```bash
git diff L00..L01 --stat
```

---

# 第十九阶段：L01 最终口试

不看资料回答：

1. `/dev/ttyUSB0` 是普通文件吗？为什么可以 open？
2. `crw-rw---- root dialout` 怎么解释？
3. `fd=4` 代表什么？
4. `O_NOCTTY` 为什么适合串口程序？
5. `O_NONBLOCK` 为什么和 poll 配合？
6. 什么是 115200 8N1？
7. `tcgetattr` 与 `tcsetattr` 分别干什么？
8. 为什么 `cfsetispeed` 后还要 `tcsetattr`？
9. raw mode 解决什么问题？
10. `read()` 一次等不等于一条消息？为什么？
11. `write()` 为什么可能不是一次写完？
12. `poll()` 返回 0 代表什么？
13. POLLIN / POLLHUP / POLLERR 各表示什么？
14. 主线程和接收线程共享哪些进程资源？
15. 什么是临界资源？
16. 为什么 Logger 需要 mutex？
17. Ctrl+C 对应什么 Linux 信号？
18. `sigaction` 相比默认退出有什么价值？
19. 串口拔掉为什么应该转 OFFLINE，而不是程序退出？
20. 为什么自动重连留到 L02？

---

# 第二十阶段：进入 L02 前的硬标准

全部打勾才能进入 L02：

```text
[ ] L00 make test 仍 PASS
[ ] make build 无编译错误
[ ] make list 能扫描真实 ttyUSB/ttyACM（有硬件时）
[ ] 能解释设备文件和文件描述符
[ ] 能看懂 SerialPort::open_port
[ ] 能解释 termios 配置顺序
[ ] 能解释 115200 8N1 raw
[ ] 能解释 O_RDWR / O_NOCTTY / O_NONBLOCK
[ ] 能解释 read/write 的返回值
[ ] 能解释 poll 的作用
[ ] 能解释 receiver thread 的职责
[ ] 能解释三个 mutex 为什么存在
[ ] 能解释 SIGINT 与 sigaction
[ ] make sim + petlink_l01 能完整收发
[ ] face/servo/status/raw PING 均通过
[ ] 模拟器退出后 L01 不崩溃
[ ] make test 全部 PASS
[ ] 使用 strace 看过 open/read/write/poll/ioctl/close
[ ] 使用 GDB 在 open_port/write_text 停过断点
[ ] 使用 ps -L 或 GDB info threads 看过线程
[ ] Git 已提交并打 L01 标签
```

完成这些后，L02 才开始：

```text
二进制帧
CRC16
粘包拆包
序号
ACK
超时重发
错误统计
自动重连
```
