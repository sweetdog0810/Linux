#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

mkdir -p logs
rm -f logs/petlink_l00.log

OUTPUT_FILE="$(mktemp)"
trap 'rm -f "$OUTPUT_FILE"' EXIT

printf 'face happy\nservo 120\nlight 2048\nstatus\nsysinfo\nservo 999\nbad_command\nquit\n' \
    | ./build/petlink_l00 > "$OUTPUT_FILE"

grep -q '表情已切换为 HAPPY' "$OUTPUT_FILE"
grep -q '舵机角度已设置为 120 度' "$OUTPUT_FILE"
grep -q 'Light value: 2048' "$OUTPUT_FILE"
grep -q -- '--- Linux System Report ---' "$OUTPUT_FILE"
grep -q '舵机角度必须在 0～180 度之间' "$OUTPUT_FILE"
grep -q '未知命令：bad_command' "$OUTPUT_FILE"
test -s logs/petlink_l00.log
grep -q 'PetLink L00 started' logs/petlink_l00.log
grep -q 'PetLink L00 stopped by user' logs/petlink_l00.log

echo 'L00 automatic self-check: PASS'
