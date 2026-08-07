# L01 生成后自检结果

生成项目后已实际执行：

```bash
make test
```

结果：

```text
L00 automatic self-check: PASS
L01 static self-check: PASS
L01 PTY integration self-check: PASS
L01 disconnect self-check: PASS
```

覆盖：

- L00 回归功能
- termios 关键 API
- `poll()`
- 接收线程
- mutex
- `sigaction()`
- ttyUSB/ttyACM 扫描
- PTY 伪串口真实 `open/termios/write/read`
- face/servo/status 双向收发
- 串口突然断开后程序继续存活
- 正常资源清理退出
