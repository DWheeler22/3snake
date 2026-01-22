#!/bin/bash
# Test 3snake with commands that prompt for password

cd /home/dj/PurpleTeam/3snake/3snake

echo "[*] Building fresh..."
make clean > /dev/null 2>&1
make > /dev/null 2>&1

if [ ! -f ./3snake ]; then
    echo "[!] Build failed!"
    exit 1
fi

echo "[*] Starting tracer..."
timeout 45 sudo ./3snake 2>&1 | tee /tmp/3snake_run.log &
TRACER_PID=$!

sleep 2

echo "[*] Test 1: sudo su (WILL prompt for password)"
echo "testpass123" | sudo su root -c "whoami" 2>/dev/null || true
sleep 2

echo "[*] Test 2: sudo passwd (WILL prompt for current password)"
echo -e "testpass123\nnewpass\nnewpass" | sudo passwd 2>/dev/null || true
sleep 2

wait $TRACER_PID 2>/dev/null || true

echo ""
echo "[*] Output summary:"
echo "---"
grep -E "\[TRACE\]|\[SUDO\]|\[SU\]|\[CRED\]|\[PASSWD\]|Found SYSCALL_read" /tmp/3snake_run.log | tail -50
