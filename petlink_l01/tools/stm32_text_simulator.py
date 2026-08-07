#!/usr/bin/env python3
"""PetLink L01 STM32 文本协议模拟器。

运行后创建一对伪终端。把打印出的 /dev/pts/N 作为 petlink_l01 的 --port 参数。
本脚本持有 PTY master，模拟 STM32 对文本命令作出回复。
"""

import os
import pty
import select
import signal
import sys

running = True


def stop_handler(_signum, _frame):
    global running
    running = False


signal.signal(signal.SIGINT, stop_handler)
signal.signal(signal.SIGTERM, stop_handler)

master_fd, slave_fd = pty.openpty()
slave_name = os.ttyname(slave_fd)

state = {
    "mode": "normal",
    "servo": 90,
    "light": 1823,
}

print("PetLink L01 STM32 simulator")
print(f"Pseudo serial device: {slave_name}")
print("Open another terminal and run:")
print(f"  ./build/petlink_l01 --port {slave_name} --baud 115200")
print("Press Ctrl+C here to stop simulator.\n")
sys.stdout.flush()

pending = b""


def reply(text: str):
    os.write(master_fd, (text + "\n").encode())
    print(f"SIM TX> {text}")
    sys.stdout.flush()


try:
    while running:
        readable, _, _ = select.select([master_fd], [], [], 0.25)
        if not readable:
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
            print(f"SIM RX> {line}")
            sys.stdout.flush()

            if line.startswith("FACE:"):
                state["mode"] = line.split(":", 1)[1].lower()
                reply(f"STM32: OK FACE={state['mode'].upper()}")
            elif line.startswith("SERVO:"):
                try:
                    value = int(line.split(":", 1)[1])
                except ValueError:
                    reply("STM32: ERROR BAD_SERVO")
                    continue
                state["servo"] = value
                reply(f"STM32: OK SERVO={value}")
            elif line.startswith("LIGHTSIM:"):
                try:
                    state["light"] = int(line.split(":", 1)[1])
                    reply(f"STM32: OK LIGHT={state['light']}")
                except ValueError:
                    reply("STM32: ERROR BAD_LIGHT")
            elif line == "STATUS?":
                reply(
                    f"STATE mode={state['mode']} light={state['light']} servo={state['servo']}"
                )
            elif line == "PING":
                reply("PONG")
            else:
                reply("STM32: ERROR UNKNOWN_COMMAND")
finally:
    os.close(master_fd)
    os.close(slave_fd)
    print("Simulator stopped.")
