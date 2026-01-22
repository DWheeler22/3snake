#!/bin/bash
cd /home/dj/PurpleTeam/3snake/3snake

echo "[*] Rebuilding..."
make clean > /dev/null 2>&1
make > /dev/null 2>&1

echo "[*] Starting tracer with 25 second timeout"
timeout 25 sudo ./3snake 2>&1 > /tmp/output.txt &
PID=$!

sleep 2

echo "[*] Test 1: sudo passwd root - entering password 'testpass123'"
printf "testpass123\n" | sudo passwd root 2>/dev/null || true

sleep 3

kill $PID 2>/dev/null || wait $PID 2>/dev/null || true

echo ""
echo "=== Full Output ==="
cat /tmp/output.txt

echo ""
echo "=== Credentials Found ==="
grep -E "\[CRED\]" /tmp/output.txt || echo "No [CRED] messages found"

echo ""
echo "=== Summary ==="
grep -E "\[TRACE\]|type=" /tmp/output.txt | tail -5
