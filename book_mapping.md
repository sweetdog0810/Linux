# L00 与《野火 Linux 基础与应用开发实战指南》的对应关系

| L00 学习任务 | 对应章节/小节 | 在项目中的体现 |
|---|---|---|
| 家目录、根目录、`/home`、`/dev`、`/proc`、`/sys` | 第7章；7.1、7.2、7.3、7.3.5、7.3.7、7.3.8、7.3.9 | 工程放在家目录；`sysinfo`读取`/proc`；系统报告扫描`/dev` |
| 用户、用户组、权限 | 第8章；8.1、8.2 | `id`、`groups`、`ls -l`；脚本可执行权限 |
| 终端和常用命令 | 第9章；9.1～9.6 | 建目录、查看文件、运行程序、重定向、`man` |
| apt安装工具 | 第10章；10.1、10.2 | 安装`build-essential`、`cmake`、`gdb`、`git` |
| VS Code与Vim | 第11章；11.2、11.3 | 使用VS Code工作区、Tasks、GDB配置；会最基本Vim退出/保存 |
| Git | 第21章；21.1～21.4 | 初始化仓库、提交、查看历史、打`L00`标签 |
| GCC和编译过程 | 第23章；23.1、23.2、23.4 | 编译C++源文件，理解源文件到可执行文件 |
| MCU与Linux程序运行差异 | 第25章；25.1、25.2 | 程序由Linux加载为进程，不需要烧录进CPU Flash |
| Makefile | 第26章；26.1、26.2 | 使用`make build/run/test/clean` |
| Makefile目标、依赖和伪目标 | 第27章；27.2～27.6 | `build`、`run`、`test`、`.PHONY` |
| 文件与系统调用 | 第28章；28.2～28.8 | Logger使用`open/write/close`；SystemInfo使用`open/read/close` |

> CMake、GDB和VS Code的`tasks.json/launch.json`属于本项目的工程化补充，不是这本书相应章节的完整教学内容。
