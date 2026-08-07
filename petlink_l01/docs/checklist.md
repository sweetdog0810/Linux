# PetLink L01 检查表

## A. L00 基线
- [ ] `make test` -> L00 PASS
- [ ] Git 中存在 L00 提交
- [ ] Git 中存在 `L00` tag

## B. USB / VMware / /dev
- [ ] USB 转 TTL 已连接给 Ubuntu 虚拟机
- [ ] `lsusb` 可以看到 USB 串口设备
- [ ] `/dev/ttyUSB0` 或 `/dev/ttyACM0` 出现
- [ ] 会使用 `sudo dmesg -w` 看插拔
- [ ] 会使用 `stty -F <device> -a`

## C. 权限
- [ ] 会看 `ls -l /dev/ttyUSB0`
- [ ] 能解释字符设备 `c`
- [ ] 会用 `groups` / `id`
- [ ] 知道 dialout 的作用
- [ ] 最终不依赖 sudo 运行 PetLink

## D. SerialFinder
- [ ] 知道 glob 用来扫描 ttyUSB/ttyACM
- [ ] 知道 access 只是权限提示
- [ ] `make list` 正常

## E. SerialPort / termios
- [ ] 能解释 `open()`
- [ ] 能解释 fd
- [ ] 能解释 O_RDWR
- [ ] 能解释 O_NOCTTY
- [ ] 能解释 O_NONBLOCK
- [ ] 能解释 115200 8N1
- [ ] 能解释 raw mode
- [ ] 能解释 tcgetattr
- [ ] 能解释 cfsetispeed/cfsetospeed
- [ ] 能解释 tcsetattr
- [ ] 能解释 tcflush

## F. I/O / poll
- [ ] 知道 read 返回值含义
- [ ] 知道 write 不保证一次写完
- [ ] 能解释 POLLIN
- [ ] 能解释 POLLOUT
- [ ] 能解释 POLLHUP/POLLERR/POLLNVAL
- [ ] 知道 poll timeout 返回 0

## G. 线程 / mutex
- [ ] 能说明主线程职责
- [ ] 能说明 RX 线程职责
- [ ] 知道线程共享进程资源
- [ ] 知道为什么 logger 要 mutex
- [ ] 知道为什么串口 write/close 要同步
- [ ] 能正常 join 线程

## H. 信号
- [ ] 知道 Ctrl+C -> SIGINT
- [ ] 知道 `sigaction()` 用途
- [ ] 知道 handler 里只设置退出标志
- [ ] Ctrl+C 后能正常清理并退出

## I. 模拟器
- [ ] `make sim` 成功
- [ ] 能连接 `/dev/pts/N`
- [ ] `face happy` 收发成功
- [ ] `servo 120` 收发成功
- [ ] `status` 收发成功
- [ ] `raw PING` -> PONG
- [ ] 关闭模拟器后应用不崩溃

## J. 调试
- [ ] 用 strace 看过 open/read/write/poll/ioctl/close
- [ ] 用 GDB 断在 `SerialPort::open_port`
- [ ] 用 GDB 断在 `SerialPort::write_text`
- [ ] 用 `info threads` 看过线程
- [ ] 用 `ps -L` 看过线程

## K. 自动验收
- [ ] `make test-l00` PASS
- [ ] `make test-l01` PASS
- [ ] `make test` 全 PASS

## L. Git
- [ ] `git status` 干净或修改已确认
- [ ] 提交信息 `L01: complete Linux serial console`
- [ ] `git tag L01`
- [ ] 会用 `git diff L00..L01 --stat`
