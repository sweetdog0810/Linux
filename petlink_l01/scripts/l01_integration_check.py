#!/usr/bin/env python3
import os
import pty
import select
import subprocess
import threading
import time
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
APP = os.path.join(ROOT, "build", "petlink_l01")

master_fd, slave_fd = pty.openpty()
slave_name = os.ttyname(slave_fd)
stop = threading.Event()
state = {"mode": "normal", "servo": 90, "light": 1823}


def simulator():
    pending = b""
    while not stop.is_set():
        try:
            ready, _, _ = select.select([master_fd], [], [], 0.1)
        except (OSError, ValueError):
            return
        if not ready:
            continue
        try:
            data = os.read(master_fd, 512)
        except OSError:
            continue
        if not data:
            continue
        pending += data
        while b"\n" in pending:
            raw, pending = pending.split(b"\n", 1)
            line = raw.rstrip(b"\r").decode(errors="replace")
            if line.startswith("FACE:"):
                state["mode"] = line.split(":", 1)[1].lower()
                response = f"STM32: OK FACE={state['mode'].upper()}\n"
            elif line.startswith("SERVO:"):
                state["servo"] = int(line.split(":", 1)[1])
                response = f"STM32: OK SERVO={state['servo']}\n"
            elif line.startswith("LIGHTSIM:"):
                state["light"] = int(line.split(":", 1)[1])
                response = f"STM32: OK LIGHT={state['light']}\n"
            elif line == "STATUS?":
                response = (
                    f"STATE mode={state['mode']} light={state['light']} "
                    f"servo={state['servo']}\n"
                )
            else:
                response = "STM32: ERROR UNKNOWN_COMMAND\n"
            os.write(master_fd, response.encode())


thread = threading.Thread(target=simulator, daemon=True)
thread.start()

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
for command in ["face happy", "servo 120", "light 2048", "status"]:
    proc.stdin.write(command + "\n")
    proc.stdin.flush()
    time.sleep(0.2)

time.sleep(0.5)
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
    raise SystemExit("[FAIL] petlink_l01 did not exit")
finally:
    stop.set()
    thread.join(timeout=1)
    os.close(master_fd)
    os.close(slave_fd)

checks = [
    (rc == 0, "program exits with code 0"),
    ("Connected to" in output, "serial opens PTY"),
    ("TX> FACE:HAPPY" in output, "face command transmitted"),
    ("RX> STM32: OK FACE=HAPPY" in output, "face response received"),
    ("TX> SERVO:120" in output, "servo command transmitted"),
    ("RX> STM32: OK SERVO=120" in output, "servo response received"),
    ("TX> STATUS?" in output, "status query transmitted"),
    ("RX> STATE mode=happy light=2048 servo=120" in output, "status response received"),
    ("PetLink L01 已正常退出" in output, "normal resource cleanup"),
]

failed = False
for ok, label in checks:
    if ok:
        print(f"[PASS] {label}")
    else:
        failed = True
        print(f"[FAIL] {label}")

if failed:
    print("\n--- captured output ---")
    print(output)
    sys.exit(1)

print("L01 PTY integration self-check: PASS")
