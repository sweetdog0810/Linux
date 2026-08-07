# PetLink L00 学习指南

## 项目目标

完成一个运行于Ubuntu的命令行桌宠程序。它暂时不连接STM32，但会建立V2所需的第一层能力：

1. Linux目录和终端操作；
2. 多文件C++工程；
3. GCC/CMake/Makefile构建；
4. GDB调试；
5. Git版本管理；
6. `open/read/write/close`系统调用；
7. 命令解析、状态管理和日志模块。

## 建议学习节奏

不要一次把完整代码全背下来。按下面顺序学习，每完成一关再进入下一关。

### 第0关：只观察，不修改

```bash
pwd
ls -la
find . -maxdepth 2 -type f | sort
```

画出目录树，并回答：头文件为什么放`include/`，实现为什么放`src/`？

### 第1关：生成Linux系统报告

```bash
bash scripts/system_report.sh
cat reports/system_report.txt
```

重点观察：当前目录、用户、UID/GID、内核、`/proc/meminfo`和`/dev`设备。

### 第2关：手动编译最小版本

先只编译全部源文件：

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude \
    src/main.cpp src/command_parser.cpp src/device_state.cpp \
    src/logger.cpp src/system_info.cpp -o petlink_l00_manual
./petlink_l00_manual
```

确认手动编译成功后再使用CMake。

### 第3关：使用CMake和Makefile

```bash
make build
make run
```

再手动执行底层CMake命令：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/petlink_l00
```

### 第4关：逐个测试命令

```text
help
status
face happy
servo 120
light 2048
status
sysinfo
servo 999
bad_command
quit
```

### 第5关：观察日志系统调用

```bash
cat logs/petlink_l00.log
strace -e trace=openat,read,write,close ./build/petlink_l00
```

在程序中输入`status`和`quit`，观察系统调用。

### 第6关：GDB调试

```bash
gdb ./build/petlink_l00
```

依次输入：

```gdb
break main
run
next
break parse_command
continue
print input
backtrace
quit
```

### 第7关：Git保存L00

```bash
git init
git status
git add .
git commit -m "L00: complete Linux command-line pet"
git log --oneline
git tag L00
```

### 第8关：自动自检

```bash
make test
```

必须看到：

```text
L00 automatic self-check: PASS
```

## 必做改造练习

1. 增加`face sad`，需要修改枚举、解析和状态显示。
2. 增加`reset`，把表情、舵机和光照恢复默认值。
3. 增加`pid`命令，只显示当前进程PID。
4. 故意将日志路径改成无权限目录，观察`open()`失败。
5. 故意把`servo`范围判断删除，确认自检或人工测试能发现问题。
