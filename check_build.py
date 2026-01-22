#!/usr/bin/env python3
import subprocess
import os

os.chdir('/home/dj/PurpleTeam/3snake/3snake')

# Try to compile
print("[*] Attempting to build...")
result = subprocess.run(['make', 'clean'], capture_output=True, timeout=10)
result = subprocess.run(['make'], capture_output=True, timeout=60)

if result.returncode != 0:
    print("[!] Build failed!")
    print("STDOUT:", result.stdout.decode())
    print("STDERR:", result.stderr.decode())
    exit(1)

# Check if binary exists
if os.path.exists('./3snake'):
    print("[+] Binary created successfully!")
    print("[+] File size:", os.path.getsize('./3snake'), "bytes")
else:
    print("[!] Binary not found after build!")
    exit(1)
