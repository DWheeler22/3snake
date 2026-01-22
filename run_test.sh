#!/bin/bash
# Simple build and test script

cd /home/dj/PurpleTeam/3snake/3snake

# Build
echo "[*] Building..."
make clean >/dev/null 2>&1
make 2>&1

# Check if built successfully
if [ ! -f ./3snake ]; then
    echo "[!] Build failed!"
    exit 1
fi

echo "[+] Build successful!"
echo "[*] Testing with debug output..."
echo "[*] Running: sudo -l (should output debug info)"
timeout 5 sudo ./3snake &
sleep 1
sudo -l > /dev/null
sleep 2
pkill -f "sudo ./3snake"
echo "[*] Test complete"
