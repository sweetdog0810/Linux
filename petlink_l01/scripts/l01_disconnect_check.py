#!/usr/bin/env python3
import os
import pty
import subprocess
import time
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
APP = os.path.join(ROOT, "build", "petlink_l01")
master_fd, slave_fd = pty.openpty()
slave_name = os.ttyname(slave_fd)

proc = subprocess.Popen(
    [APP, "--port", slave_name, "--baud", "115200"],
    cwd=ROOT,
    stdin=subprocess.PIPE,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
    text=True,
    bufsize=1,
)
assert proc.stdin is not None

time.sleep(0.5)
os.close(master_fd)  # 模拟 USB 串口突然被拔掉。
time.sleep(0.8)
proc.stdin.write("serial\n")
proc.stdin.flush()
time.sleep(0.2)
proc.stdin.write("quit\n")
proc.stdin.flush()
proc.stdin.close()

try:
    output = proc.stdout.read() if proc.stdout is not None else ""
    rc = proc.wait(timeout=8)
except subprocess.TimeoutExpired:
    proc.kill()
    output = proc.stdout.read() if proc.stdout is not None else ""
    print(output)
    sys.exit("[FAIL] disconnect test timed out")
finally:
    os.close(slave_fd)

checks = [
    (rc == 0, "program survives disconnect"),
    ("串口已断开" in output, "disconnect is detected"),
    ("Serial: OFFLINE" in output, "state becomes OFFLINE"),
    ("PetLink L01 已正常退出" in output, "program can still quit normally"),
]
failed = False
for ok, label in checks:
    print(("[PASS] " if ok else "[FAIL] ") + label)
    failed |= not ok
if failed:
    print("\n--- captured output ---")
    print(output)
    sys.exit(1)
print("L01 disconnect self-check: PASS")
