#!/usr/bin/env bash
set -euo pipefail

need() {
    local pattern="$1"
    local file="$2"
    local label="$3"
    if ! grep -q "$pattern" "$file"; then
        echo "[FAIL] missing $label in $file"
        exit 1
    fi
    echo "[PASS] $label"
}

need 'tcgetattr' src/serial_port.cpp 'termios tcgetattr'
need 'tcsetattr' src/serial_port.cpp 'termios tcsetattr'
need 'cfsetispeed' src/serial_port.cpp 'input baud configuration'
need 'cfsetospeed' src/serial_port.cpp 'output baud configuration'
need '::poll' src/serial_port.cpp 'poll system call'
need 'std::thread' src/l01_main.cpp 'receiver thread'
need 'std::mutex' src/l01_main.cpp 'console mutex'
need 'sigaction' src/l01_main.cpp 'SIGINT/SIGTERM handler'
need '/dev/ttyUSB' src/serial_finder.cpp 'ttyUSB discovery'
need '/dev/ttyACM' src/serial_finder.cpp 'ttyACM discovery'

echo 'L01 static self-check: PASS'
