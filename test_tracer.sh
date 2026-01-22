#!/bin/bash

# Test script for 3snake with added debug information

cd /home/dj/PurpleTeam/3snake/3snake

echo "================================"
echo "3snake Tracing Test"
echo "================================"
echo ""

if [ ! -f ./3snake ]; then
    echo "[!] Binary ./3snake not found!"
    echo "[*] Building..."
    make clean
    make
fi

echo "[*] Starting 3snake in terminal mode with debug output..."
echo "[*] Debug messages will appear below with [DEBUG] prefix"
echo "[*] Traced passwords will appear with [username] timestamp prefix"
echo ""
echo "Press Ctrl+C to stop"
echo ""
echo "In another terminal, run one of these commands:"
echo "  sudo -l"
echo "  sudo su"
echo "  sudo passwd"
echo ""

timeout 30 sudo ./3snake 2>&1 | tee /tmp/3snake_test.log &
TRACER_PID=$!

sleep 2
echo "[*] Tracer started (PID: $TRACER_PID)"
echo "[*] Now run a sudo/su command in another terminal..."
echo "[*] (Waiting 25 more seconds...)"

wait $TRACER_PID

echo ""
echo "[*] Test complete. Check output above for results."
echo "[*] Full log saved to: /tmp/3snake_test.log"
