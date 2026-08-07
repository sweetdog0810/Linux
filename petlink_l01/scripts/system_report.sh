#!/usr/bin/env bash
set -u

mkdir -p reports
REPORT="reports/system_report.txt"

{
    echo "===== PetLink L00 System Report ====="
    echo "Time: $(date '+%F %T')"
    echo
    echo "[Current directory]"
    pwd
    echo
    echo "[Current user]"
    whoami
    id
    echo
    echo "[Kernel]"
    uname -a
    echo
    echo "[Operating system]"
    cat /etc/os-release
    echo
    echo "[Memory summary]"
    grep -E '^(MemTotal|MemFree|MemAvailable):' /proc/meminfo
    echo
    echo "[Possible serial devices]"
    find /dev -maxdepth 1 \( -name 'ttyUSB*' -o -name 'ttyACM*' \) -print 2>/dev/null || true
    echo
    echo "[Project permissions]"
    ls -ld . include src scripts docs logs
} > "$REPORT"

echo "System report generated: $REPORT"
