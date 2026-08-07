<<<<<<< HEAD
# PetLink L00：Linux命令行桌宠

这是PetLink Linux学习链的第一个版本。它不连接STM32，目标是先建立Linux应用开发的基本工程能力。

## 快速开始

```bash
sudo apt update
sudo apt install -y build-essential cmake gdb git strace

make build
make run
```

程序命令：

```text
help
status
face happy
servo 120
light 2048
sysinfo
quit
```

自动检查：

```bash
make test
```

生成Linux系统报告：

```bash
bash scripts/system_report.sh
cat reports/system_report.txt
```

详细学习顺序见：

- `docs/L00_learning_guide.md`
- `docs/book_mapping.md`
- `docs/checklist.md`
=======
# Linux
>>>>>>> 3d4e3635491655c63e642a52ce36cabe352af7b5
