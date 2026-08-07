# L00 完成检查表

## A. Linux目录与命令
- [ ] 我知道`~`表示当前用户家目录。
- [ ] 我能解释绝对路径和相对路径。
- [ ] 我能解释`/home`、`/dev`、`/proc`、`/sys`的用途。
- [ ] 我能使用`pwd`、`cd`、`ls -la`、`mkdir`、`touch`、`cp`、`mv`、`rm`。
- [ ] 我能使用`cat`、`less`、`grep`、`find`。
- [ ] 我知道Tab补全和Ctrl+C取消当前命令。

## B. 用户与权限
- [ ] 我能使用`whoami`、`id`、`groups`。
- [ ] 我能看懂`ls -l`中`rwx`的基本含义。
- [ ] 我能解释为什么脚本需要执行权限。
- [ ] 我没有习惯性地用`sudo`运行普通项目程序。

## C. 编译与工程
- [ ] 我能解释源文件、目标文件、可执行文件。
- [ ] 我能执行`make build`、`make run`、`make clean`。
- [ ] 我能解释Makefile中的目标、依赖、命令和`.PHONY`。
- [ ] 我能执行CMake配置和构建命令。

## D. 程序理解
- [ ] 我能指出主循环在哪里。
- [ ] 我能说明命令从输入到修改DeviceState的流程。
- [ ] 我能说明Logger为什么使用文件描述符。
- [ ] 我知道`open()`失败时返回负数。
- [ ] 我知道`write()`不应被假设为永远一次写完。
- [ ] 我能解释`sysinfo`读取了哪些Linux信息。

## E. 调试和版本管理
- [ ] 我能用GDB在`main`设置断点。
- [ ] 我能用`next`、`step`、`print`、`backtrace`。
- [ ] 我能使用`git status/add/commit/log/diff`。
- [ ] 我完成了`git tag L00`。

## F. 功能验收
- [ ] `face happy`有效。
- [ ] `servo 0`、`servo 180`有效。
- [ ] `servo -1`、`servo 181`被拒绝。
- [ ] `light 0`、`light 4095`有效。
- [ ] 空输入和未知命令不会崩溃。
- [ ] `sysinfo`能显示PID、UID、内核和内存。
- [ ] 日志文件能够记录正常命令和错误命令。
- [ ] `make test`显示`PASS`。
