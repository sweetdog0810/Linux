# PetLink Linux Learning — L01 串口系统编程版

L01 不是重新开一个完全无关的工程，而是在 L00 的基础上加入 Linux 串口系统编程能力。

## 本版本学习目标

- `/dev/ttyUSB*`、`/dev/ttyACM*` 设备发现
- 文件描述符与 `open/read/write/close`
- `termios`：115200、8N1、raw mode
- `poll()` 等待串口/标准输入事件
- 接收线程与互斥保护
- `SIGINT/SIGTERM` 正常退出
- 串口拔出检测（L01 不自动重连）
- 日志与错误处理
- 伪终端 PTY 模拟 STM32，无硬件也可测试

## 构建

```bash
make build
```

## 无硬件自测

```bash
make test
```

应同时看到 L00 回归测试、L01 静态检查、PTY 收发测试和断线测试全部 PASS。

## 使用模拟 STM32

终端 A：

```bash
make sim
```

它会打印一个 `/dev/pts/N`。

终端 B：

```bash
./build/petlink_l01 --port /dev/pts/N --baud 115200
```

然后尝试：

```text
face happy
servo 120
status
raw PING
quit
```

## 使用真实 USB 串口

```bash
make list
./build/petlink_l01 --port /dev/ttyUSB0 --baud 115200
```

注意：L01 使用临时文本协议训练串口系统编程。L02 才进入二进制帧、CRC、粘包拆包、ACK/重发与自动重连。

完整学习流程见 `docs/L01_learning_guide.md`。
