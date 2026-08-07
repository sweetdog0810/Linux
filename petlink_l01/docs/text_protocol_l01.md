# L01 临时文本协议

本协议只为学习 Linux 串口系统编程服务，不是最终 V2 协议。

## Linux -> STM32

| 用户命令 | 串口实际发送 |
|---|---|
| `face happy` | `FACE:HAPPY\n` |
| `servo 90` | `SERVO:90\n` |
| `status` | `STATUS?\n` |
| `light 2048` | `LIGHTSIM:2048\n`，仅学习模拟器使用 |
| `raw PING` | `PING\n` |

## 模拟器 -> Linux

示例：

```text
STM32: OK FACE=HAPPY
STM32: OK SERVO=90
STATE mode=happy light=1823 servo=90
PONG
```

L01 的重点是 `open/termios/read/write/poll/thread/signal`，所以协议故意保持简单。
L02 将替换为正式二进制帧格式。
