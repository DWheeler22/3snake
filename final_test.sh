#!/bin/bash
cd /home/dj/PurpleTeam/3snake/3snake
make clean > /dev/null 2>&1
make > /dev/null 2>&1

echo "[*] Testing passwd credential capture"
timeout 20 sudo ./3snake 2>&1 &
TRACER=$!

sleep 1

echo "[*] Running: echo testpass | sudo passwd root"
echo "testpass" | sudo passwd root 2>/dev/null || true

sleep 2
kill $TRACER 2>/dev/null || wait $TRACER

echo ""
echo "[*] Looking for credential output..."
